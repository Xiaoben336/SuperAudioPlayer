//
// Created by zjf on 2019/1/4.
//

#include "JfCallJava.h"

JfCallJava::JfCallJava(JavaVM *vm, JNIEnv *env, jobject *obj) {
    this->javaVM = vm;
    this->jniEnv = env;
    this->jobj = env->NewGlobalRef(*obj);//设为全局

    jclass jclz = env->GetObjectClass(jobj);
    if (!jclz) {
        LOGE("get jclass error");
        return ;
    }

    jmid_prepared = env->GetMethodID(jclz,"onCallPrepared","()V");
    jmid_loading = env->GetMethodID(jclz,"onCallLoading","(Z)V");
}

JfCallJava::~JfCallJava() {

}

void JfCallJava::onCallPrepared(int threadType) {
    if (threadType == MAIN_THREAD){
        jniEnv->CallVoidMethod(jobj,jmid_prepared);
    } else if (threadType == CHILD_THREAD){
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv,0) != JNI_OK){
            if (LOG_DEBUG) {
                LOGE("GET CHILD THREAD JNIENV ERROR");
                return;
            }
        }

        jniEnv->CallVoidMethod(jobj,jmid_prepared);
        javaVM->DetachCurrentThread();
    }
}

void JfCallJava::onCallLoading(int threadType, bool loading) {
    if (threadType == MAIN_THREAD){
        jniEnv->CallVoidMethod(jobj,jmid_loading,loading);
    } else if (threadType == CHILD_THREAD){
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv,0) != JNI_OK){
            if (LOG_DEBUG) {
                LOGE("GET CHILD THREAD JNIENV ERROR");
                return;
            }
        }

        jniEnv->CallVoidMethod(jobj,jmid_loading,loading);
        javaVM->DetachCurrentThread();
    }
}


