//
// Created by vito_li on 2017/3/7.
//

#ifndef SECURITYMONITOR_CAMERA_LOG_H
#define SECURITYMONITOR_CAMERA_LOG_H

#include <android/log.h>

#define  LOG_TAG    "camerahook"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#endif //SECURITYMONITOR_CAMERA_LOG_H
