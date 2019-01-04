//
// Created by zjf on 2019/1/4.
//

#ifndef SUPERAUDIOPLAYER_ANDROIDLOG_H
#define SUPERAUDIOPLAYER_ANDROIDLOG_H
#endif //SUPERAUDIOPLAYER_ANDROIDLOG_H

#include <jni.h>
#include <android/log.h>
#define TAG "JFPlayer"

#define LOG_DEBUG true
#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_DEBUG, TAG, FORMAT, ##__VA_ARGS__);
#define LOGW(FORMAT,...) __android_log_print(ANDROID_LOG_WARN, TAG, FORMAT, ##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR, TAG, FORMAT, ##__VA_ARGS__);