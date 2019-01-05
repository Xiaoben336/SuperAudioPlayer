//
// Created by zjf on 2019/1/4.
//

#ifndef SUPERAUDIOPLAYER_JFAUDIO_H
#define SUPERAUDIOPLAYER_JFAUDIO_H

#include "JfQueue.h"
#include "JfPlayStatus.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
extern "C"{
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
};


class JfAudio {

public:
    int streamIndex = -1;//stream索引
    AVCodecParameters *codecpar = NULL;//包含音视频参数的结构体。很重要，可以用来获取音视频参数中的宽度、高度、采样率、编码格式等信息。
    AVCodecContext *pACodecCtx = NULL;

    JfQueue *queue = NULL;
    JfPlayStatus *playStatus = NULL;

    pthread_t playThread;
    AVPacket *avPacket = NULL;
    AVFrame *avFrame = NULL;//解码出来的帧
    int ret = -1;

    uint8_t *buffer = NULL;
    int data_size;//buffer size
    int sample_rate = 0;//保存输入音频文件OpenSL ES的采样率

    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine = NULL;

    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvReb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    SLObjectItf pcmPlayerObject = NULL;
    SLPlayItf pcmPlayerPlay = NULL;

    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;

public:
    JfAudio(JfPlayStatus *playStatus,int sample_rate);
    ~JfAudio();

    void play();//播放
    int resampleAudio();//返回重采样的大小，用于求时间

    void initOpenSLES();

    uint getCurrentSampleRateForOpenSLES(int sample_rate);
};


#endif //SUPERAUDIOPLAYER_JFAUDIO_H
