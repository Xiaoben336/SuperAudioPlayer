//
// Created by zjf on 2019/1/4.
//

#ifndef SUPERAUDIOPLAYER_JFCALLJAVA_H
#define SUPERAUDIOPLAYER_JFCALLJAVA_H

#include <jni.h>
#include "AndroidLog.h"

#define MAIN_THREAD 0
#define CHILD_THREAD 1
/**
 * C++层调用Java层的类
 */
class JfCallJava {

public:
    JavaVM *javaVM = NULL;
    JNIEnv *jniEnv = NULL;
    jobject jobj;

    jmethodID jmid_prepared;
    jmethodID jmid_loading;
    jmethodID jmid_timeinfo;
    jmethodID jmid_error;
    jmethodID jmid_complete;
    jmethodID jmid_renderyuv;
    jmethodID jmid_issupcodec;
    jmethodID jmid_initmediacodec;
    jmethodID jmid_decodeavpacket;
public:
    JfCallJava(JavaVM *vm,JNIEnv *env,jobject *obj);
    ~JfCallJava();

    void onCallPrepared(int threadType);//onCallPrepare的调用方法
    void onCallLoading(int threadType, bool loading);//onCallLoading的调用方法
    void onCallTimeInfo(int threadType,int currentTime,int totalTime);
    void onCallError(int threadType,int code, char *msg);
    void onCallComplete(int threadType);
    void onCallRenderYUV(int threadType,int width,int height,uint8_t *fy,uint8_t *fu,uint8_t *fv);
    bool onCallIsSupCodec(const char *ffcodecname);
    void onCallInitMediaCodec(const char *mime,int width,int height,uint8_t csd0_size,uint8_t csd1_size,uint8_t *csd_0,uint8_t *csd_1);
    void onCallDecodeAVPacket(int datasize,uint8_t *packetdata);
};


#endif //SUPERAUDIOPLAYER_JFCALLJAVA_H
