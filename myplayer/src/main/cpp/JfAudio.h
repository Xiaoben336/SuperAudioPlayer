//
// Created by zjf on 2019/1/4.
//

#ifndef SUPERAUDIOPLAYER_JFAUDIO_H
#define SUPERAUDIOPLAYER_JFAUDIO_H

#include "JfQueue.h"
#include "JfPlayStatus.h"
extern "C"{
#include <libavcodec/avcodec.h>
};


class JfAudio {

public:
    int streamIndex = -1;//stream索引
    AVCodecParameters *codecpar = NULL;//包含音视频参数的结构体。很重要，可以用来获取音视频参数中的宽度、高度、采样率、编码格式等信息。
    AVCodecContext *pACodecCtx = NULL;

    JfQueue *queue = NULL;
    JfPlayStatus *playStatus = NULL;
public:
    JfAudio(JfPlayStatus *playStatus );
    ~JfAudio();
};


#endif //SUPERAUDIOPLAYER_JFAUDIO_H
