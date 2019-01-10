//
// Created by zjf on 2019/1/4.
//

#ifndef SUPERAUDIOPLAYER_JFQUEUE_H
#define SUPERAUDIOPLAYER_JFQUEUE_H


#include "queue"
#include "pthread.h"
#include "AndroidLog.h"
#include "JfPlayStatus.h"
extern "C"{
    #include <libavcodec/avcodec.h>
};
class JfQueue {

public:
    std::queue<AVPacket *> queuePacket;//存储AVPacket的队列
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;
    JfPlayStatus *jfPlayStatus = NULL;

public:
    JfQueue(JfPlayStatus *jfPlayStatus);
    ~JfQueue();

    int putAVPacket(AVPacket *avPacket);//将AVPacket放进队列中
    int getAVPacket(AVPacket *avPacket);//从队列中取出AVPacket
    int getQueueSize();

    void clearAVPacket();
};


#endif //SUPERAUDIOPLAYER_JFQUEUE_H
