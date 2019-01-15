//
// Created by zjf on 2019/1/10.
//


#include "JfVideo.h"

JfVideo::JfVideo(JfPlayStatus *playStatus, JfCallJava *callJava) {
    this->playStatus = playStatus;
    this->callJava = callJava;
    queue = new JfQueue(playStatus);
    pthread_mutex_init(&codec_mutex,NULL);
}

JfVideo::~JfVideo() {
    pthread_mutex_destroy(&codec_mutex);
}


void *playVideo(void *data){
    JfVideo *video = (JfVideo *)data;

    while (video->playStatus != NULL && !video->playStatus->exit){
        if (video->playStatus->seeking){
            av_usleep(1000 * 100);
            continue;
        }

        if (video->playStatus->pausing){//暂停状态
            av_usleep(1000 * 100);
            continue;
        }

        if (video->queue->getQueueSize() == 0){//加载状态
            if (!video->playStatus->loading){
                video->playStatus->loading = true;
                video->callJava->onCallLoading(CHILD_THREAD, true);
                LOGD("VIDEO加载状态");
            }
            av_usleep(1000 * 100);
            continue;
        } else {
            if (video->playStatus->loading){
                video->playStatus->loading = false;
                video->callJava->onCallLoading(CHILD_THREAD, false);
                LOGD("VIDEO播放状态");
            }
        }

        /*AVPacket *avPacket = av_packet_alloc();
        if (video->queue->getAVPacket(avPacket) == 0){
            //解码渲染
            LOGD("线程中获取视频AVPacket");
        }
        av_packet_free(&avPacket);//AVPacket中的第一个参数，就是引用，减到0才真正释放
        av_free(avPacket);
        avPacket = NULL;*/

        AVPacket *avPacket = av_packet_alloc();
        if (video->queue->getAVPacket(avPacket) != 0){
            av_packet_free(&avPacket);//AVPacket中的第一个参数，就是引用，减到0才真正释放
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }


        if (video->codectype == CODEC_MEDIACODEC){
            LOGD("硬解码视频");
            //送入AVPacket过滤
            if (av_bsf_send_packet(video->abs_ctx,avPacket) != 0) {
                av_packet_free(&avPacket);//AVPacket中的第一个参数，就是引用，减到0才真正释放
                av_free(avPacket);
                avPacket = NULL;
                continue;
            }
            //接收过滤后的AVPacket
            while (av_bsf_receive_packet(video->abs_ctx,avPacket) == 0){
                LOGD("开始解码");

                double diff = video->getFrameDiffTime(NULL,avPacket);
                LOGD("DIFF IS %f",diff);
                av_usleep(video->getDelayTime(diff) * 1000000);

                video->callJava->onCallDecodeAVPacket(avPacket->size,avPacket->data);

                av_packet_free(&avPacket);//AVPacket中的第一个参数，就是引用，减到0才真正释放
                av_free(avPacket);
                continue;
            }
            avPacket = NULL;
        }
        if (video->codectype == CODEC_YUV){
            LOGD("软解码视频");
            pthread_mutex_lock(&video->codec_mutex);
            if (avcodec_send_packet(video->pVCodecCtx,avPacket) != 0){

                av_packet_free(&avPacket);//AVPacket中的第一个参数，就是引用，减到0才真正释放
                av_free(avPacket);
                avPacket = NULL;
                pthread_mutex_unlock(&video->codec_mutex);
                continue;
            }
            AVFrame *avFrame = av_frame_alloc();
            if (avcodec_receive_frame(video->pVCodecCtx,avFrame) != 0){
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = NULL;
                av_packet_free(&avPacket);//AVPacket中的第一个参数，就是引用，减到0才真正释放
                av_free(avPacket);
                avPacket = NULL;
                pthread_mutex_unlock(&video->codec_mutex);
                continue;
            }

            if (LOG_DEBUG){
                LOGD("子线程解码一个AVFrame成功");
            }

            if (avFrame->format == AV_PIX_FMT_YUV420P){
                //直接渲染
                LOGD("YUV420P");

                double diff = video->getFrameDiffTime(avFrame,NULL);

                LOGD("DIFF IS %f",diff);

                av_usleep(video->getDelayTime(diff) * 1000000);

                video->callJava->onCallRenderYUV(
                        CHILD_THREAD,
                        video->pVCodecCtx->width,
                        video->pVCodecCtx->height,
                        avFrame->data[0],
                        avFrame->data[1],
                        avFrame->data[2]);
            } else {
                //转成YUV420P
                AVFrame *pFrameYUV420P = av_frame_alloc();
                int num = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,video->pVCodecCtx->width,video->pVCodecCtx->height,1);
                uint8_t *buffer = (uint8_t *)(av_malloc(num * sizeof(uint8_t)));
                av_image_fill_arrays(
                        pFrameYUV420P->data,
                        pFrameYUV420P->linesize,
                        buffer,
                        AV_PIX_FMT_YUV420P,
                        video->pVCodecCtx->width,
                        video->pVCodecCtx->height,
                        1);
                SwsContext *sws_ctx = sws_getContext(
                        video->pVCodecCtx->width,
                        video->pVCodecCtx->height,
                        video->pVCodecCtx->pix_fmt,
                        video->pVCodecCtx->width,
                        video->pVCodecCtx->height,
                        AV_PIX_FMT_YUV420P,
                        SWS_BICUBIC,
                        NULL,NULL,NULL
                );

                if (!sws_ctx){
                    av_frame_free(&pFrameYUV420P);
                    av_free(pFrameYUV420P);
                    av_free(buffer);
                    pthread_mutex_unlock(&video->codec_mutex);
                    continue;
                }

                sws_scale(
                        sws_ctx,
                        avFrame->data,
                        avFrame->linesize,
                        0,
                        avFrame->height,
                        pFrameYUV420P->data,
                        pFrameYUV420P->linesize);//这里得到YUV数据
                LOGD("NO_YUV420P");
                //渲染

                double diff = video->getFrameDiffTime(avFrame,NULL);

                LOGD("DIFF IS %f",diff);

                av_usleep(video->getDelayTime(diff) * 1000000);


                video->callJava->onCallRenderYUV(
                        CHILD_THREAD,
                        video->pVCodecCtx->width,
                        video->pVCodecCtx->height,
                        pFrameYUV420P->data[0],
                        pFrameYUV420P->data[1],
                        pFrameYUV420P->data[2]);

                av_frame_free(&pFrameYUV420P);
                av_free(pFrameYUV420P);
                av_free(buffer);
                sws_freeContext(sws_ctx);
            }

            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            av_packet_free(&avPacket);//AVPacket中的第一个参数，就是引用，减到0才真正释放
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&video->codec_mutex);
        }
    }
    return 0;
    //pthread_exit(&video->thread_play);
}
void JfVideo::play() {
    if (playStatus != NULL && !playStatus->exit){
        pthread_create(&thread_play,NULL,playVideo,this);
    }
}

void JfVideo::release() {

    if (queue != NULL){
        queue->noticeQueue();
    }
    pthread_join(thread_play,NULL);

    if (queue != NULL){
        delete(queue);
        queue = NULL;
    }

    if (abs_ctx != NULL){
        av_bsf_free(&abs_ctx);
        av_free(abs_ctx);
        abs_ctx = NULL;
    }
    if (pVCodecCtx != NULL){
        pthread_mutex_lock(&codec_mutex);
        avcodec_close(pVCodecCtx);
        avcodec_free_context(&pVCodecCtx);
        pVCodecCtx = NULL;
        pthread_mutex_unlock(&codec_mutex);
    }

    if (playStatus != NULL){
        playStatus = NULL;
    }

    if (callJava != NULL){
        callJava = NULL;
    }
}

/**
 * 当前帧的AVFrame
 */
double JfVideo::getFrameDiffTime(AVFrame *avFrame,AVPacket *avPacket) {
    double pts = 0;
    if (avFrame != NULL){
        pts = av_frame_get_best_effort_timestamp(avFrame);
    }
    if (avPacket != NULL) {
        pts = avPacket->pts;
    }


    if (pts == AV_NOPTS_VALUE){
        pts = 0;
    }

    double timestamp = pts * av_q2d(time_base);

    if (timestamp > 0){
        clock = timestamp;
    }

    double diff = audio->clock - clock;

    return diff;
}

double JfVideo::getDelayTime(double diff) {
    if (diff > 0.003){
        delayTime = delayTime * 2 / 3;//可能会越来越快

        if (delayTime < defaultDelayTime / 2){
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2){
            delayTime = defaultDelayTime * 2;
        }

    } else if (diff < -0.003){
        delayTime = delayTime * 3 / 2;

        if (delayTime < defaultDelayTime / 2){
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2){
            delayTime = defaultDelayTime * 2;
        }

    } else if (diff == 0.003){

    }

    if (diff >= 0.5){
        delayTime = 0;
    } else if (diff <= -0.5){
        delayTime = defaultDelayTime * 2;
    }

    if (fabs(diff) >= 10){//音频不存在
        delayTime = defaultDelayTime;
    }
    return delayTime;
}
