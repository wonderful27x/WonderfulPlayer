//
// Created by Acer on 2019/12/31.
//

#ifndef WYWONDERFULPLAYER_JNICALLBACK_H
#define WYWONDERFULPLAYER_JNICALLBACK_H

#include <jni.h>
#include "errorType.h"

class JniCallback {
public:
    JniCallback(JavaVM *javaVm,JNIEnv *env,jobject instance);
    ~JniCallback();
    void prepareSuccess(int threadMode);
    void error(int threadMode,ErrorType type);
    void progressUpdate(int threadMode, double progress);

private:
    JavaVM *javaVm = nullptr;
    JNIEnv *env = nullptr;
    jobject instance;
    jmethodID prepareId;
    jmethodID errorId;
    jmethodID progressId;
};


#endif //WYWONDERFULPLAYER_JNICALLBACK_H
