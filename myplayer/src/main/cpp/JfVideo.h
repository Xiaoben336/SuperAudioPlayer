//
// Created by zjf on 2019/1/10.
//

#ifndef SUPERAUDIOPLAYER_JFVIDEO_H
#define SUPERAUDIOPLAYER_JFVIDEO_H

#define CODEC_YUV 0
#define CODEC_MEDIACODEC 1

#include "JfQueue.h"
#include "JfCallJava.h"
#include "JfAudio.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};

class JfVideo {
public:
    int streamIndex = -1;
    AVCodecContext *pVCodecCtx = NULL;
    AVCodecParameters *codecpar = NULL;
    JfQueue *queue = NULL;
    JfPlayStatus *playStatus = NULL;
    JfCallJava *callJava = NULL;
    AVRational time_base;

    JfAudio *audio = NULL;

    double clock = 0;
    double delayTime = 0;
    double defaultDelayTime = 0.04;
    pthread_t thread_play;

    pthread_mutex_t codec_mutex;//seek时avcodec_send_packet和avcodec_receive_frame也会操作CodecContext

    int codectype = CODEC_YUV;

    AVBSFContext *abs_ctx = NULL;
public:
    JfVideo(JfPlayStatus *playStatus,JfCallJava *callJava);
    ~JfVideo();

    void play();
    void release();

    double getFrameDiffTime(AVFrame *avFrame,AVPacket *avPacket);

    double getDelayTime(double diff);
};


#endif //SUPERAUDIOPLAYER_JFVIDEO_H
