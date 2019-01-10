//
// Created by zjf on 2019/1/3.
//
#include <jni.h>
#include "AndroidLog.h"
#include "JfCallJava.h"
#include "JfFFmpeg.h"

extern "C" {
#include <libavformat/avformat.h>
}

JavaVM *javaVM =NULL;
JfCallJava *callJava = NULL;
JfFFmpeg *ffmpeg = NULL;
JfPlayStatus *playStatus = NULL;
pthread_t thread_start;
bool nexit = true;
/**
 *   用来初始化JavaVM对象
 *
 *  JNI_OnLoad是在加载so的时候调用的，也就是System.loadLibrary("test")时候调用的
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved){
    jint ret = -1;
    javaVM = vm;
    JNIEnv *env;
    if (javaVM->GetEnv((void **)&env,JNI_VERSION_1_4) != JNI_OK){
        return ret;
    }
    return JNI_VERSION_1_4;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_myplayer_player_JfPlayer_n_1prepared(JNIEnv *env, jobject instance,
                                                      jstring source_) {
    const char *source = (env)->GetStringUTFChars(source_, 0);

    // 初始化
    if (ffmpeg == NULL){
        if (callJava == NULL){
            callJava = new JfCallJava(javaVM,env,&instance);
        }
        callJava->onCallLoading(MAIN_THREAD, true);
        playStatus = new JfPlayStatus();
        ffmpeg = new JfFFmpeg(playStatus,callJava,source);
        ffmpeg->prepare();
    }

    //(env)->ReleaseStringUTFChars(source_, source);
}


void *startCallback(void *data){
    JfFFmpeg *ffmpeg = (JfFFmpeg *)data;
    ffmpeg->start();
    pthread_exit(&thread_start);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_myplayer_player_JfPlayer_n_1start(JNIEnv *env, jobject instance) {

    // TODO
    if (ffmpeg != NULL){
        //ffmpeg->start();
        pthread_create(&thread_start,NULL,startCallback,ffmpeg);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myplayer_player_JfPlayer_n_1pause(JNIEnv *env, jobject instance) {

    // TODO
    if (ffmpeg != NULL){
        ffmpeg->pause();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myplayer_player_JfPlayer_n_1resume(JNIEnv *env, jobject instance) {

    // TODO
    if (ffmpeg != NULL){
        ffmpeg->resume();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myplayer_player_JfPlayer_n_1stop(JNIEnv *env, jobject instance) {

    if (!nexit){
        return;
    }
    // TODO

    jclass jclz = env->GetObjectClass(instance);
    jmethodID jmid_play_next = env->GetMethodID(jclz,"onCallPlayNext","()V");

    nexit = false;
    if (ffmpeg != NULL){
        ffmpeg->release();
        delete(ffmpeg);
        ffmpeg = NULL;

        if (callJava != NULL){
            delete(callJava);
            callJava = NULL;
        }
        if (playStatus != NULL){
            delete(playStatus);
            playStatus = NULL;
        }
    }
    nexit = true;

    env->CallVoidMethod(instance,jmid_play_next);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myplayer_player_JfPlayer_n_1seek(JNIEnv *env, jobject instance, jint sec) {

    // TODO
    if (ffmpeg != NULL){
        ffmpeg->seek(sec);
    }
}