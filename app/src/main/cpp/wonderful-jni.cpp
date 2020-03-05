#include <jni.h>
#include <string>
#include "WonderfulPlayer.h"
#include <android/native_window_jni.h>

extern "C"{
#include "com_example_wywonderfulplayer_WonderfulPlayer.h"
}

void destroyCall();

JavaVM *javaVm = nullptr;
WonderfulPlayer *player = nullptr;
JniCallback *jniCallback = nullptr;
ANativeWindow *window = nullptr;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//静态初始化

void renderFrame(uint8_t *data,int width,int height,int lineSize){
    pthread_mutex_lock(&mutex);
    if(!window){
        pthread_mutex_unlock(&mutex);
        return;
    }
    //初始化窗口属性
    ANativeWindow_setBuffersGeometry(window,width,height,WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer buffer;
    if(ANativeWindow_lock(window,&buffer, nullptr)){
        ANativeWindow_release(window);
        window = nullptr;
        return;
    }
    uint8_t *windowData = static_cast<uint8_t *>(buffer.bits);
    int windowDataLineSize = buffer.stride*4;
    //内存拷贝
    for (int i = 0; i <buffer.height ; ++i) {
        memcpy(windowData + i*windowDataLineSize,data + i*lineSize,windowDataLineSize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
}

jint JNI_OnLoad(JavaVM *vm,void *argc){
    javaVm = vm;
    return JNI_VERSION_1_6;
}

/*
 * Class:     com_example_wywonderfulplayer_WonderfulPlayer
 * Method:    prepareNative
 * Signature: (Ljava/lang/String;)V
 */
//初始化FFMPEG
extern "C"
JNIEXPORT void JNICALL Java_com_example_wywonderfulplayer_WonderfulPlayer_prepareNative(JNIEnv *env, jobject thizz, jstring dataSource){
    const char *path = env->GetStringUTFChars(dataSource,nullptr);
    jniCallback = new JniCallback(javaVm,env,thizz);
    player = WonderfulPlayer::getInstance();
    player->init(path);
    player->setRenderCallback(renderFrame);
    player->setJniCallback(jniCallback);
    player->setDestroyCall(destroyCall);
    player->prepare();
    env->ReleaseStringUTFChars(dataSource,path);
}

/*
 * Class:     com_example_wywonderfulplayer_WonderfulPlayer
 * Method:    startNative
 * Signature: ()V
 */
extern "C"
JNIEXPORT void JNICALL Java_com_example_wywonderfulplayer_WonderfulPlayer_startNative(JNIEnv *env, jobject thizz){
    if(player){
        player->start();
    }
}

/*
 * Class:     com_example_wywonderfulplayer_WonderfulPlayer
 * Method:    stopNative
 * Signature: ()V
 */
extern "C"
JNIEXPORT void JNICALL Java_com_example_wywonderfulplayer_WonderfulPlayer_stopNative(JNIEnv *env, jobject thizz){
    if(player){
        player->stop();
    }
}

//内存释放回掉，这是一个从内向外的内存释放
void destroyCall(){
    pthread_mutex_lock(&mutex);
    if(window){
        ANativeWindow_release(window);
        window = nullptr;
    }
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);

    if(player){
        player->setDestroyCall(nullptr);
    }
    DELETE(player);
    DELETE(jniCallback);

    LOGD("destroy-finish");
}

extern "C"
JNIEXPORT void JNICALL Java_com_example_wywonderfulplayer_WonderfulPlayer_setSurfaceNative(JNIEnv *env, jobject thizz,jobject surface){
    pthread_mutex_lock(&mutex);
    //先移除之前的window
    if(window){
        ANativeWindow_release(window);
        window = nullptr;
    }
    //创建新的window
    window = ANativeWindow_fromSurface(env,surface);
    pthread_mutex_unlock(&mutex);
}

//获取总时长
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_wywonderfulplayer_WonderfulPlayer_getDurationNative(JNIEnv *env, jobject thiz) {
    // TODO: implement getDurationNative()
    if(player){
        return static_cast<jint>(player->getDuration());
    }
    return 0;
}
//控制播放进度
extern "C"
JNIEXPORT void JNICALL
Java_com_example_wywonderfulplayer_WonderfulPlayer_seekNative(JNIEnv *env, jobject thiz,
                                                              jdouble progress) {
    // TODO: implement seekNative()
    if(player){
        player->seek(progress);
    }
}
//控制播放状态
extern "C"
JNIEXPORT void JNICALL
Java_com_example_wywonderfulplayer_WonderfulPlayer_playControlNative(JNIEnv *env, jobject thiz,
                                                                     jint state) {
    // TODO: implement playControlNative()
    PlayState  playState = static_cast<PlayState>(state);
    if(player){
        player->playControl(playState);
    }
}