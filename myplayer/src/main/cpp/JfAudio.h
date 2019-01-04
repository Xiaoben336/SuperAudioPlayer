//
// Created by zjf on 2019/1/4.
//

#ifndef SUPERAUDIOPLAYER_JFAUDIO_H
#define SUPERAUDIOPLAYER_JFAUDIO_H

extern "C"{
#include <libavcodec/avcodec.h>
};


class JfAudio {

public:
    int streamIndex = -1;//stream索引
    AVCodecParameters *codecpar = NULL;//包含音视频参数的结构体。很重要，可以用来获取音视频参数中的宽度、高度、采样率、编码格式等信息。
    AVCodecContext *pACodecCtx = NULL;
public:
    JfAudio();
    ~JfAudio();
};


#endif //SUPERAUDIOPLAYER_JFAUDIO_H
