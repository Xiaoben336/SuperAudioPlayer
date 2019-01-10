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
    jfFFmpeg->decodeAudioThread();
    pthread_exit(&jfFFmpeg->decodeThread);//退出线程
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
void JfFFmpeg::decodeAudioThread() {
    pthread_mutex_lock(&init_mutex);
    av_register_all();
    avformat_network_init();

    pAFmtCtx = avformat_alloc_context();

    pAFmtCtx->interrupt_callback.callback = avformat_callback;
    pAFmtCtx->interrupt_callback.opaque = this;

    if (avformat_open_input(&pAFmtCtx,url,NULL,NULL) != 0){
        if (LOG_DEBUG){
            LOGE("open url file error url === %s",url);
            callJava->onCallError(CHILD_THREAD,401,"open url file error url");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    if (avformat_find_stream_info(pAFmtCtx,NULL) < 0){
        if (LOG_DEBUG){
            LOGE("find stream info error url === %s",url);
            callJava->onCallError(CHILD_THREAD,402,"find stream info error url");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    for (int i = 0; i < pAFmtCtx->nb_streams; i++) {
        if (pAFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (audio == NULL) {
                audio = new JfAudio(playStatus,pAFmtCtx->streams[i]->codecpar->sample_rate,callJava);
                audio->streamIndex = i;
                audio->codecpar = pAFmtCtx->streams[i]->codecpar;
                audio->duration = pAFmtCtx->duration / AV_TIME_BASE;//单位是秒
                audio->time_base = pAFmtCtx->streams[i]->time_base;
                duration = audio->duration;
            }
        }
    }

    AVCodec *dec = avcodec_find_decoder(audio->codecpar->codec_id);
    if (!dec){
        if (LOG_DEBUG){
            LOGE("FIND DECODER ERROR");
            callJava->onCallError(CHILD_THREAD,403,"FIND DECODER ERROR");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    audio->pACodecCtx = avcodec_alloc_context3(dec);
    if (!audio->pACodecCtx){
        if (LOG_DEBUG){
            LOGE("avcodec_alloc_context3 ERROR");
            callJava->onCallError(CHILD_THREAD,404,"avcodec_alloc_context3 ERROR");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    if (avcodec_parameters_to_context(audio->pACodecCtx,audio->codecpar)){//将解码器中信息复制到上下文当中
        if (LOG_DEBUG){
            LOGE("avcodec_parameters_to_context ERROR");
            callJava->onCallError(CHILD_THREAD,405,"avcodec_parameters_to_context ERROR");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    if (avcodec_open2(audio->pACodecCtx,dec,NULL) < 0){
        if (LOG_DEBUG){
            LOGE("avcodec_open2 ERROR");
            callJava->onCallError(CHILD_THREAD,406,"avcodec_open2 ERROR");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
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

    audio->play();

    int count;
    while (playStatus != NULL && !playStatus->exit) {

        if (playStatus->seeking){
            continue;//如果seeking中，不再往下执行
        }

        if (audio->queue->getQueueSize() > 40){//设置队列只保存40 frames，
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();

        pthread_mutex_lock(&seek_mutex);
        int ret = av_read_frame(pAFmtCtx,avPacket);
        pthread_mutex_unlock(&seek_mutex);

        if (ret == 0) {
            if (avPacket->stream_index == audio->streamIndex){
                count++;
                /*if (LOG_DEBUG) {
                    LOGD("解码第%d帧",count);
                }*/
                audio->queue->putAVPacket(avPacket);
            } else {
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
                    continue;
                } else {
                    playStatus->exit = true;
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
    if (audio != NULL){
        audio->pause();
    }
}

void JfFFmpeg::resume() {
    if (audio != NULL){
        audio->resume();
    }
}

void JfFFmpeg::release() {
    /*if (playStatus->exit){
        return;
    }*/
    playStatus->exit = true;

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
    if (pAFmtCtx != NULL){
        avformat_close_input(&pAFmtCtx);
        avformat_free_context(pAFmtCtx);
        pAFmtCtx = NULL;
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
    if (duration < 0){
        return;
    }
    if (sec >= 0 && sec <= duration){
        if (audio != NULL){
            playStatus->seeking = true;
            audio->queue->clearAVPacket();//可能队列中还有一两秒的缓存，所以要清空
            audio->clock = 0;//时间置零，需要重新计算
            audio->last_time = 0;

            pthread_mutex_lock(&seek_mutex);

            int64_t rel = sec * AV_TIME_BASE;
            avformat_seek_file(pAFmtCtx,-1,INT64_MIN,rel,INT64_MAX,0);

            pthread_mutex_unlock(&seek_mutex);
            playStatus->seeking = false;
        }
    }
}
