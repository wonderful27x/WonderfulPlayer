//
// Created by Acer on 2019/12/31.
//

#ifndef WYWONDERFULPLAYER_WONDERFULPLAYER_H
#define WYWONDERFULPLAYER_WONDERFULPLAYER_H

#include "AudioChannel.h"
#include "VideoChannel.h"
#include "JniCallback.h"
#include "wonderfulEnum.h"

extern "C"{
#include <libavformat/avformat.h>
};

class WonderfulPlayer {
    friend void *stopTask(void *player);
    friend void playerDestroyCall(int type);
public:
    static WonderfulPlayer* getInstance();
    ~WonderfulPlayer();
    void init(const char *sourcePath);
    void prepare();
    void prepareExecute();
    void start();
    void startExecute();
    void stop();
    void seek(double progress);
    void playControl(PlayState state);
    void destroy(int type);
    double getDuration();
    void setJniCallback(JniCallback *jniCallback);
    void setRenderCallback(RenderCallback renderCallback);
    void setDestroyCall(void(*destroyCall)(void));

private:
    static WonderfulPlayer *wonderfulPlayer;
    WonderfulPlayer();
    WonderfulPlayer(const char *sourcePath);

    char *sourcePath = nullptr;
    AVFormatContext *formatContext = nullptr;
    AudioChannel *audioChannel = nullptr;
    VideoChannel *videoChannel = nullptr;
    JniCallback *jniCallback = nullptr;
    pthread_t pid_prepare = 0;
    pthread_t pid_start = 0;
    pthread_t pid_stop = 0;
    int audioStreamIndex = -1;
    int videoStreamIndex = -1;
//    int isPlaying = 0;
    double duration = 0;                //总时间长
    pthread_mutex_t formatContextMutex; //AVFormatContext的互斥锁
    pthread_mutex_t destroyMutex;       //释放函数的互斥锁
    PlayState playState = STOP;         //播放状态
    RenderCallback renderCallback;
    void (*destroyCall)(void);          //内存释放回掉
};


#endif //WYWONDERFULPLAYER_WONDERFULPLAYER_H
