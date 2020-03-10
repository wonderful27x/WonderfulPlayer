//
// Created by Acer on 2019/12/31.
//

#include "JniCallback.h"
#include "macro.h"
#include <jni.h>

JniCallback::JniCallback(JavaVM *javaVm,JNIEnv *env,jobject instance) {
    this->javaVm = javaVm;
    this->env = env;
    this->instance = env->NewGlobalRef(instance);
    jclass clazz = env->GetObjectClass(instance);
    this->prepareId = env->GetMethodID(clazz,"prepareSuccess","()V");
    this->errorId = env->GetMethodID(clazz,"error","(I)V");
    this->progressId = env->GetMethodID(clazz,"progressUpdate","(I)V");
}

JniCallback::~JniCallback() {
    javaVm = nullptr;
    env->DeleteGlobalRef(instance);
    env = nullptr;
    instance = 0;
    prepareId = 0;
    errorId = 0;
    progressId = 0;
}

void JniCallback::prepareSuccess(int threadMode) {
    //主线程jni回调
    if(threadMode == THREAD_MAIN){
        env->CallVoidMethod(instance,prepareId);
    }
    //子线程的接口回调不能使用主线程的env,需要通过JavaVM拿到子线程的env
    else if(threadMode == THREAD_CHILD){
        JNIEnv *childEnv;
        javaVm->AttachCurrentThread(&childEnv,nullptr);
        childEnv->CallVoidMethod(instance,prepareId);
        javaVm->DetachCurrentThread();
    }
}

void JniCallback::error(int threadMode,ErrorType type) {
    //主线程jni回调
    if(threadMode == THREAD_MAIN){
        env->CallVoidMethod(instance,errorId,type);
    }
    //子线程的接口回调不能使用主线程的env,需要通过JavaVM拿到子线程的env
    else if(threadMode == THREAD_CHILD){
        JNIEnv *childEnv;
        javaVm->AttachCurrentThread(&childEnv,nullptr);
        childEnv->CallVoidMethod(instance,errorId,type);
        javaVm->DetachCurrentThread();
    }
}

void JniCallback::progressUpdate(int threadMode,int progress) {
    //主线程jni回调
    if(threadMode == THREAD_MAIN){
        env->CallVoidMethod(instance,progressId,progress);
    }
    //子线程的接口回调不能使用主线程的env,需要通过JavaVM拿到子线程的env
    else if(threadMode == THREAD_CHILD){
        JNIEnv *childEnv;
        javaVm->AttachCurrentThread(&childEnv,nullptr);
        childEnv->CallVoidMethod(instance,progressId,progress);
        javaVm->DetachCurrentThread();
    }
}
