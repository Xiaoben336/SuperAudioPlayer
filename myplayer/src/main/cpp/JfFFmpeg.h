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
#include <libavutil/time.h>
};

class JfFFmpeg {

public:
    JfCallJava *callJava = NULL;
    const char *url = NULL;//文件的url
    pthread_t decodeThread = NULL;//解码的子线程
    pthread_mutex_t init_mutex;
    pthread_mutex_t seek_mutex;
    bool exit = false;
    int duration = 0;
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
    void release();

    void seek(int64_t sec);
};


#endif //SUPERAUDIOPLAYER_JFFFMPEG_H
