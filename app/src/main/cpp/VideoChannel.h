//
// Created by Acer on 2019/12/31.
//

#ifndef WYWONDERFULPLAYER_VIDEOCHANNEL_H
#define WYWONDERFULPLAYER_VIDEOCHANNEL_H


#include "BaseChannel.h"
#include "macro.h"
#include "AudioChannel.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
};

typedef void(*RenderCallback)(uint8_t *,int,int,int);

class VideoChannel : public BaseChannel{
public:
    VideoChannel(int streamIndex,AVCodecContext *codecContext,AVRational time_base,int fps);
    ~VideoChannel();
    void start();
    void stop();
    void decode();
    void play();
    void setRenderCallback(RenderCallback renderCallback);
    void setAudioChannel(AudioChannel *audioChannel);

private:
    RenderCallback renderCallback = nullptr;
    pthread_t pid_video_decode;    //解码线程pid
    pthread_t pid_video_play;      //播放线程pid
    int fps;                       //帧率
    AudioChannel *audioChannel = nullptr;
};


#endif //WYWONDERFULPLAYER_VIDEOCHANNEL_H
