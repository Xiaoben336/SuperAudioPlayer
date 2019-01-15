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
    jmid_timeinfo = env->GetMethodID(jclz,"onCallTimeInfo","(II)V");
    jmid_error = env->GetMethodID(jclz,"onCallError","(ILjava/lang/String;)V");
    jmid_complete = env->GetMethodID(jclz,"onCallComplete","()V");
    jmid_renderyuv = env->GetMethodID(jclz,"onCallRenderYUV","(II[B[B[B)V");
    jmid_issupcodec = env->GetMethodID(jclz,"onCallIsSupCodec","(Ljava/lang/String;)Z");
    jmid_initmediacodec = env->GetMethodID(jclz,"onCallInitMediaCodec","(Ljava/lang/String;II[B[B)V");
    jmid_decodeavpacket = env->GetMethodID(jclz,"onCallDecodeAVPacket","(I[B)V");
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

void JfCallJava::onCallTimeInfo(int threadType, int currentTime, int totalTime) {
    if (threadType == MAIN_THREAD){
        jniEnv->CallVoidMethod(jobj,jmid_timeinfo,currentTime,totalTime);
    } else if (threadType == CHILD_THREAD){
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv,0) != JNI_OK){
            if (LOG_DEBUG) {
                LOGE("GET CHILD THREAD JNIENV ERROR");
                return;
            }
        }

        jniEnv->CallVoidMethod(jobj,jmid_timeinfo,currentTime,totalTime);
        javaVM->DetachCurrentThread();
    }
}

void JfCallJava::onCallError(int threadType, int code, char *msg) {
    if (threadType == MAIN_THREAD){
        jstring jmsg = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jobj,jmid_error,code,jmsg);
        jniEnv->DeleteLocalRef(jmsg);
    } else if (threadType == CHILD_THREAD){
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv,0) != JNI_OK){
            if (LOG_DEBUG) {
                LOGE("GET CHILD THREAD JNIENV ERROR");
                return;
            }
        }
        jstring jmsg = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jobj,jmid_error,code,jmsg);
        jniEnv->DeleteLocalRef(jmsg);
        javaVM->DetachCurrentThread();
    }
}

void JfCallJava::onCallComplete(int threadType) {
    if (threadType == MAIN_THREAD){
        jniEnv->CallVoidMethod(jobj,jmid_complete);
    } else if (threadType == CHILD_THREAD){
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv,0) != JNI_OK){
            if (LOG_DEBUG) {
                LOGE("GET CHILD THREAD JNIENV ERROR");
                return;
            }
        }

        jniEnv->CallVoidMethod(jobj,jmid_complete);
        javaVM->DetachCurrentThread();
    }
}

void JfCallJava::onCallRenderYUV(int threadType, int width, int height, uint8_t *fy, uint8_t *fu,
                                 uint8_t *fv) {
    if (threadType == MAIN_THREAD){
        jbyteArray y = jniEnv->NewByteArray(width * height);
        jniEnv->SetByteArrayRegion(y,0,width * height,(jbyte *)fy);

        jbyteArray u = jniEnv->NewByteArray(width * height / 4);
        jniEnv->SetByteArrayRegion(u,0,width * height / 4,(jbyte *)fu);

        jbyteArray v = jniEnv->NewByteArray(width * height / 4);
        jniEnv->SetByteArrayRegion(v,0,width * height / 4,(jbyte *)fv);

        jniEnv->CallVoidMethod(jobj,jmid_renderyuv,width,height,y,u,v);

        jniEnv->DeleteLocalRef(y);
        jniEnv->DeleteLocalRef(u);
        jniEnv->DeleteLocalRef(v);

    } else if (threadType == CHILD_THREAD){
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv,0) != JNI_OK){
            if (LOG_DEBUG) {
                LOGE("GET CHILD THREAD JNIENV ERROR");
                return;
            }
        }

        jbyteArray y = jniEnv->NewByteArray(width * height);
        jniEnv->SetByteArrayRegion(y, 0,width * height, reinterpret_cast<const jbyte *>(fy));

        jbyteArray u = jniEnv->NewByteArray(width * height / 4);
        jniEnv->SetByteArrayRegion(u, 0,width * height / 4, reinterpret_cast<const jbyte *>(fu));

        jbyteArray v = jniEnv->NewByteArray(width * height / 4);
        jniEnv->SetByteArrayRegion(v, 0,width * height / 4, reinterpret_cast<const jbyte *>(fv));

        jniEnv->CallVoidMethod(jobj,jmid_renderyuv,width,height,y,u,v);

        jniEnv->DeleteLocalRef(y);
        jniEnv->DeleteLocalRef(u);
        jniEnv->DeleteLocalRef(v);

        javaVM->DetachCurrentThread();
    }
}

bool JfCallJava::onCallIsSupCodec(const char *ffcodecname) {
    bool supported = false;
    JNIEnv *jniEnv;
    if (javaVM->AttachCurrentThread(&jniEnv,0) != JNI_OK){
        if (LOG_DEBUG) {
            LOGE("GET CHILD THREAD JNIENV ERROR");
            return supported;
        }
    }

    jstring jtype = jniEnv->NewStringUTF(ffcodecname);
    supported = jniEnv->CallBooleanMethod(jobj,jmid_issupcodec,jtype);
    jniEnv->DeleteLocalRef(jtype);
    javaVM->DetachCurrentThread();
    return supported;
}

void JfCallJava::onCallInitMediaCodec(const char *mime, int width, int height,uint8_t csd0_size,uint8_t csd1_size, uint8_t *csd_0,
                                      uint8_t *csd_1) {

    JNIEnv *jniEnv;
    if (javaVM->AttachCurrentThread(&jniEnv,0) != JNI_OK){
        if (LOG_DEBUG) {
            LOGE("GET CHILD THREAD JNIENV ERROR");
        }
    }

    jstring type = jniEnv->NewStringUTF(mime);
    jbyteArray csd0 = jniEnv->NewByteArray(csd0_size);
    jniEnv->SetByteArrayRegion(csd0, 0, csd0_size, reinterpret_cast<const jbyte *>(csd_0));
    jbyteArray csd1 = jniEnv->NewByteArray(csd1_size);
    jniEnv->SetByteArrayRegion(csd1, 0, csd1_size, reinterpret_cast<const jbyte *>(csd_1));

    jniEnv->CallVoidMethod(jobj,jmid_initmediacodec,type,width,height,csd0,csd1);

    jniEnv->DeleteLocalRef(csd0);
    jniEnv->DeleteLocalRef(csd1);
    jniEnv->DeleteLocalRef(type);

    javaVM->DetachCurrentThread();
}

void JfCallJava::onCallDecodeAVPacket(int datasize, uint8_t *packetdata) {
    JNIEnv *jniEnv;
    if (javaVM->AttachCurrentThread(&jniEnv,0) != JNI_OK){
        if (LOG_DEBUG) {
            LOGE("GET CHILD THREAD JNIENV ERROR");
        }
    }

    jbyteArray data = jniEnv->NewByteArray(datasize);
    jniEnv->SetByteArrayRegion(data, 0, datasize, reinterpret_cast<const jbyte *>(packetdata));
    jniEnv->CallVoidMethod(jobj,jmid_decodeavpacket,datasize,data);
    jniEnv->DeleteLocalRef(data);

    javaVM->DetachCurrentThread();
}


