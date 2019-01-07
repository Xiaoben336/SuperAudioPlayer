//
// Created by zjf on 2019/1/4.
//

#include "JfFFmpeg.h"

JfFFmpeg::JfFFmpeg(JfPlayStatus *playStatus,JfCallJava *callJava, const char *url) {
    this->playStatus = playStatus;
    this->callJava = callJava;
    this->url = url;
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

void JfFFmpeg::decodeAudioThread() {
    av_register_all();
    avformat_network_init();

    pAFmtCtx = avformat_alloc_context();

    if (avformat_open_input(&pAFmtCtx,url,NULL,NULL) != 0){
        if (LOG_DEBUG){
            LOGE("open url file error url === %s",url);
        }
        return;
    }

    if (avformat_find_stream_info(pAFmtCtx,NULL) < 0){
        if (LOG_DEBUG){
            LOGE("find stream info error url === %s",url);
        }
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
            }
        }
    }

    AVCodec *dec = avcodec_find_decoder(audio->codecpar->codec_id);
    if (!dec){
        if (LOG_DEBUG){
            LOGE("FIND DECODER ERROR");
        }
        return;
    }

    audio->pACodecCtx = avcodec_alloc_context3(dec);
    if (!audio->pACodecCtx){
        if (LOG_DEBUG){
            LOGE("avcodec_alloc_context3 ERROR");
        }
        return;
    }

    if (avcodec_parameters_to_context(audio->pACodecCtx,audio->codecpar)){//将解码器中信息复制到上下文当中
        if (LOG_DEBUG){
            LOGE("avcodec_parameters_to_context ERROR");
        }
        return;
    }

    if (avcodec_open2(audio->pACodecCtx,dec,NULL) < 0){
        if (LOG_DEBUG){
            LOGE("avcodec_open2 ERROR");
        }
        return;
    }

    callJava->onCallPrepared(CHILD_THREAD);
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
        AVPacket *avPacket = av_packet_alloc();
        if (av_read_frame(pAFmtCtx,avPacket) == 0) {
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
