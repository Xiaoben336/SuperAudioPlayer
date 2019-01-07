//
// Created by zjf on 2019/1/4.
//

#ifndef SUPERAUDIOPLAYER_JFFFMPEG_H
#define SUPERAUDIOPLAYER_JFFFMPEG_H

#include "JfCallJava.h"
#include "JfAudio.h"
#include "pthread.h"

extern "C"{
#include <libavformat/avformat.h>
};

class JfFFmpeg {

public:
    JfCallJava *callJava = NULL;
    const char *url = NULL;//文件的url
    pthread_t decodeThread = NULL;//解码的子线程

    /**
     * 解码相关
     */
    AVFormatContext *pAFmtCtx = NULL;
    JfAudio *audio = NULL;

    JfPlayStatus *playStatus = NULL;
public:
    JfFFmpeg(JfPlayStatus *playStatus,JfCallJava *callJava,const char *url);//参数都是从外面传进来的
    ~JfFFmpeg();

    void prepare();
    void decodeAudioThread();

    void start();
    void pause();
    void resume();
};


#endif //SUPERAUDIOPLAYER_JFFFMPEG_H
