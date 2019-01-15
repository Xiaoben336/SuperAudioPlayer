//
// Created by zjf on 2019/1/4.
//

#include "JfFFmpeg.h"

JfFFmpeg::JfFFmpeg(JfPlayStatus *playStatus,JfCallJava *callJava, const char *url) {
    this->playStatus = playStatus;
    this->callJava = callJava;
    this->url = url;
    exit = false;
    pthread_mutex_init(&init_mutex,NULL);
    pthread_mutex_init(&seek_mutex,NULL);
}

JfFFmpeg::~JfFFmpeg() {
    pthread_mutex_destroy(&init_mutex);
    pthread_mutex_destroy(&seek_mutex);
}

void *decodeFFmpeg(void *data){
    JfFFmpeg *jfFFmpeg = (JfFFmpeg *)(data);
    jfFFmpeg->decodeFFmpegThread();
    return 0;
    //pthread_exit(&jfFFmpeg->decodeThread);//退出线程
}
/**
 * 正式解码的过程，开一个子线程解码
 */
void JfFFmpeg::prepare() {
    pthread_create(&decodeThread,NULL,decodeFFmpeg,this);
}


int avformat_callback(void *ctx){
    JfFFmpeg *jfFFmpeg = (JfFFmpeg *)ctx;
    if (jfFFmpeg->playStatus->exit){
        return AVERROR_EOF;
    }
    return 0;
}
void JfFFmpeg::decodeFFmpegThread() {
    pthread_mutex_lock(&init_mutex);
    av_register_all();
    avformat_network_init();

    pFmtCtx = avformat_alloc_context();

    pFmtCtx->interrupt_callback.callback = avformat_callback;
    pFmtCtx->interrupt_callback.opaque = this;

    if (avformat_open_input(&pFmtCtx,url,NULL,NULL) != 0){
        if (LOG_DEBUG){
            LOGE("open url file error url === %s",url);
            callJava->onCallError(CHILD_THREAD,401,"open url file error url");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    if (avformat_find_stream_info(pFmtCtx,NULL) < 0){
        if (LOG_DEBUG){
            LOGE("find stream info error url === %s",url);
            callJava->onCallError(CHILD_THREAD,402,"find stream info error url");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    for (int i = 0; i < pFmtCtx->nb_streams; i++) {
        if (pFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (audio == NULL) {
                audio = new JfAudio(playStatus,pFmtCtx->streams[i]->codecpar->sample_rate,callJava);
                audio->streamIndex = i;
                audio->codecpar = pFmtCtx->streams[i]->codecpar;
                audio->duration = pFmtCtx->duration / AV_TIME_BASE;//单位是秒
                audio->time_base = pFmtCtx->streams[i]->time_base;
                duration = audio->duration;
            }
        } else if (pFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            if (video == NULL){
                video = new JfVideo(playStatus,callJava);
                video->streamIndex = i;
                video->codecpar = pFmtCtx->streams[i]->codecpar;
                video->time_base = pFmtCtx->streams[i]->time_base;


                int num = pFmtCtx->streams[i]->avg_frame_rate.num;
                int den = pFmtCtx->streams[i]->avg_frame_rate.den;
                if (num != 0 && den != 0){
                    int fps = num / den;//[25 / 1]???????
                    video->defaultDelayTime = 1.0 / fps;
                }
            }
        }
    }

    if (audio != NULL){
        initCodecContext(audio->codecpar,&audio->pACodecCtx);
    }

    if (video != NULL){
        initCodecContext(video->codecpar,&video->pVCodecCtx);
    }
    if (callJava != NULL){
        if (playStatus != NULL && !playStatus->exit){
            callJava->onCallPrepared(CHILD_THREAD);
        } else {
            exit = true;
        }
    }
    pthread_mutex_unlock(&init_mutex);
}

void JfFFmpeg::start() {
    if (audio == NULL) {
        if (LOG_DEBUG){
            LOGE("AUDIO == NULL");
        }
    }
    if (video == NULL) {
        if (LOG_DEBUG){
            LOGE("VIDEO == NULL");
        }
    }
    supMediaCodec = false;
    video->audio = audio;

    const char *codecName = ((const AVCodec*) video->pVCodecCtx->codec)->name;
    supMediaCodec = callJava->onCallIsSupCodec(codecName);

    if (supMediaCodec){
        LOGD("当前设备支持硬解码当前视频");
        //找到相应解码器的过滤器
        if (strcasecmp(codecName,"h264") == 0) {
            bsFilter = av_bsf_get_by_name("h264_mp4toannexb");
        } else if (strcasecmp(codecName,"h265") == 0){
            bsFilter = av_bsf_get_by_name("hevc_mp4toannexb");
        }
        if (bsFilter == NULL){
            goto end;
        }

        //初始化过滤器上下文：
        if (av_bsf_alloc(bsFilter,&video->abs_ctx) != 0){
            supMediaCodec = false;
            goto end;
        }

        //添加解码器属性
        if (avcodec_parameters_copy(video->abs_ctx->par_in,video->codecpar) < 0){
            supMediaCodec = false;
            av_bsf_free(&video->abs_ctx);
            video->abs_ctx = NULL;
            goto end;
        }

        //初始化过滤器上下文
        if (av_bsf_init(video->abs_ctx) != 0){
            supMediaCodec = false;
            av_bsf_free(&video->abs_ctx);
            video->abs_ctx = NULL;
            goto end;
        }

        video->abs_ctx->time_base_in = video->time_base;
    }


    end:
    if (supMediaCodec){
        video->codectype = CODEC_MEDIACODEC;
        video->callJava->onCallInitMediaCodec(
                codecName,
                video->pVCodecCtx->width,
                video->pVCodecCtx->height,
                video->pVCodecCtx->extradata_size,
                video->pVCodecCtx->extradata_size,
                video->pVCodecCtx->extradata,
                video->pVCodecCtx->extradata
                );
    }

    audio->play();
    video->play();
    while (playStatus != NULL && !playStatus->exit) {

        if (playStatus->seeking){
            av_usleep(1000 * 100);
            continue;//如果seeking中，不再往下执行
        }

        if (audio->queue->getQueueSize() > 40){//设置队列只保存40 frames，
            av_usleep(1000 * 100);
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();

        pthread_mutex_lock(&seek_mutex);
        int ret = av_read_frame(pFmtCtx,avPacket);//seek时可能会出错
        pthread_mutex_unlock(&seek_mutex);

        if (ret == 0) {
            if (avPacket->stream_index == audio->streamIndex){
                /*if (LOG_DEBUG) {
                    LOGD("解码第%d帧",count);
                }*/
                audio->queue->putAVPacket(avPacket);
            } else if (avPacket->stream_index == video->streamIndex){
                video->queue->putAVPacket(avPacket);
                //LOGD("获取到视频流AVPacket")
            }else {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
            }
        } else {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            //队列中的avPacket还没有解码完
            while (playStatus != NULL && !playStatus->exit){
                if (audio->queue->getQueueSize() > 0){//把缓存中的avPacket也要释放出来
                    av_usleep(1000 * 100);
                    continue;
                } else {//seek失败是会走这里
                    if (!playStatus->seeking){
                        av_usleep(1000 * 500);//如果是最后一帧能播放100ms,就直接退出，则那100ms的音频和视频就播放不出来了
                        playStatus->exit = true;
                    }
                    break;
                }
            }
        }
    }

    if (callJava != NULL){
        callJava->onCallComplete(CHILD_THREAD);
    }
    exit = true;
/*
    while (audio->queue->getQueueSize() > 0){
        AVPacket *avPacket = av_packet_alloc();
        audio->queue->getAVPacket(avPacket);
        av_packet_free(&avPacket);
        av_free(avPacket);
        avPacket = NULL;
    }
*/

    if (LOG_DEBUG){
        LOGD("解码完成");
    }
}

void JfFFmpeg::pause() {
    if (playStatus != NULL){
        playStatus->pausing = true;
    }

    if (audio != NULL){
        audio->pause();
    }
}

void JfFFmpeg::resume() {
    if (playStatus != NULL){
        playStatus->pausing = false;
    }
    if (audio != NULL){
        audio->resume();
    }
}

void JfFFmpeg::release() {
    /*if (playStatus->exit){
        return;
    }*/
    playStatus->exit = true;

    pthread_join(decodeThread,NULL);

    pthread_mutex_lock(&init_mutex);

    int sleepCount = 0;
    while (!exit){
        if (sleepCount > 1000){
            exit = true;
        }
        if (LOG_DEBUG){
            LOGD("WAIT FFMPEG EXIT %d",sleepCount);
        }
        sleepCount++;
        av_usleep(1000 * 10);//10ms
    }

    if (audio != NULL){
        audio->release();
        delete(audio);
        audio = NULL;
    }
    LOGD("释放audio");
    if (video != NULL){
        video->release();
        delete(video);
        video = NULL;
    }
    LOGD("释放video");
    if (pFmtCtx != NULL){
        avformat_close_input(&pFmtCtx);
        avformat_free_context(pFmtCtx);
        pFmtCtx = NULL;
    }
    if (playStatus != NULL){
        playStatus = NULL;
    }
    if (callJava != NULL) {
        callJava = NULL;
    }
    pthread_mutex_unlock(&init_mutex);
}

void JfFFmpeg::seek(int64_t sec) {
    LOGD("sec === %ld",sec);
    if (duration < 0){
        return;
    }
    playStatus->seeking = true;
    pthread_mutex_lock(&seek_mutex);

    int64_t rel = sec * AV_TIME_BASE;
    LOGD("rel === %ld",sec);
    avformat_seek_file(pFmtCtx,-1,INT64_MIN,rel,INT64_MAX,0);

    if (sec >= 0 && sec <= duration){
        if (audio != NULL){
            audio->queue->clearAVPacket();//可能队列中还有一两秒的缓存，所以要清空
            audio->clock = 0;//时间置零，需要重新计算
            audio->last_time = 0;
            pthread_mutex_lock(&audio->codec_mutex);
            avcodec_flush_buffers(audio->pACodecCtx);
            pthread_mutex_unlock(&audio->codec_mutex);
        }

        if (video != NULL){
            video->queue->clearAVPacket();
            video->clock = 0;
            pthread_mutex_lock(&video->codec_mutex);
            avcodec_flush_buffers(video->pVCodecCtx);
            pthread_mutex_unlock(&video->codec_mutex);
        }
        pthread_mutex_unlock(&seek_mutex);
        playStatus->seeking = false;
    }
}


/**
 *
 */
int JfFFmpeg::initCodecContext(AVCodecParameters *codecParameters, AVCodecContext **pCodecContext) {
    AVCodec *dec = avcodec_find_decoder(codecParameters->codec_id);
    if (!dec){
        if (LOG_DEBUG){
            LOGE("FIND DECODER ERROR");
            callJava->onCallError(CHILD_THREAD,403,"FIND DECODER ERROR");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    *pCodecContext = avcodec_alloc_context3(dec);
    if (!(*pCodecContext)){
        if (LOG_DEBUG){
            LOGE("avcodec_alloc_context3 ERROR");
            callJava->onCallError(CHILD_THREAD,404,"avcodec_alloc_context3 ERROR");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    if (avcodec_parameters_to_context(*pCodecContext,codecParameters) < 0){//将解码器中信息复制到上下文当中
        if (LOG_DEBUG){
            LOGE("avcodec_parameters_to_context ERROR");
            callJava->onCallError(CHILD_THREAD,405,"avcodec_parameters_to_context ERROR");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    if (avcodec_open2(*pCodecContext,dec,NULL) != 0){
        if (LOG_DEBUG){
            LOGE("avcodec_open2 ERROR");
            callJava->onCallError(CHILD_THREAD,406,"avcodec_open2 ERROR");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    return 0;
}
