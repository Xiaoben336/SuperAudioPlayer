//
// Created by zjf on 2019/1/4.
//

#include "JfQueue.h"

JfQueue::JfQueue(JfPlayStatus *jfPlayStatus) {

    this->jfPlayStatus = jfPlayStatus;
    pthread_mutex_init(&mutexPacket,NULL);
    pthread_cond_init(&condPacket,NULL);
}

JfQueue::~JfQueue() {
    clearAVPacket();
    pthread_mutex_destroy(&mutexPacket);
    pthread_cond_destroy(&condPacket);
}

int JfQueue::putAVPacket(AVPacket *avPacket) {
    pthread_mutex_lock(&mutexPacket);

    queuePacket.push(avPacket);
    /*if (LOG_DEBUG){
        LOGD("放入一个AVPacket到队列中，个数为 == %d",queuePacket.size());
    }*/

    pthread_cond_signal(&condPacket);//入队完之后发一个信号

    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int JfQueue::getAVPacket(AVPacket *packet) {
    pthread_mutex_lock(&mutexPacket);

    while (jfPlayStatus != NULL && !jfPlayStatus->exit){
        if (queuePacket.size() > 0){
            AVPacket *avPacket = queuePacket.front();//取出来
            if (av_packet_ref(packet,avPacket) == 0){//把pkt的内存数据拷贝到avPacket内存中，只是拷贝了引用
                queuePacket.pop();
            }
            av_packet_free(&avPacket);//AVPacket中的第一个参数，就是引用，减到0才真正释放
            av_free(avPacket);
            avPacket = NULL;

            /*if (LOG_DEBUG){
                LOGD("从队列中取出一个AVPacket，还剩下%d个",queuePacket.size());
            }*/

            break;
        } else {
            pthread_cond_wait(&condPacket,&mutexPacket);
        }
    }

    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int JfQueue::getQueueSize() {
    int size = 0;
    pthread_mutex_lock(&mutexPacket);
    size = queuePacket.size();
    pthread_mutex_unlock(&mutexPacket);
    return size;
}

void JfQueue::clearAVPacket() {
    //有可能释放资源时线程还在加锁中
    pthread_cond_signal(&condPacket);

    pthread_mutex_lock(&mutexPacket);

    while (!queuePacket.empty()){
        //先出队再释放
        AVPacket *packet = queuePacket.front();
        queuePacket.pop();
        av_packet_free(&packet);
        av_free(packet);
        packet = NULL;
    }
    pthread_mutex_unlock(&mutexPacket);
}

void JfQueue::noticeQueue() {
    pthread_cond_signal(&condPacket);
}
