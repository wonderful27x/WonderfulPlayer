//
// Created by Acer on 2019/12/31.
//

#ifndef WYWONDERFULPLAYER_AUDIOCHANNEL_H
#define WYWONDERFULPLAYER_AUDIOCHANNEL_H

#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "androidlog.h"
#include "macro.h"

extern "C"{
#include <libswresample/swresample.h>
#include <libavutil/time.h>
};

/**
 * 音频播放使用的是OpenSLES，这是android的一个底层系统库
 */
class AudioChannel : public BaseChannel{
public:
    AudioChannel(int streamIndex,AVCodecContext *codecContext,AVRational time_base);
    ~AudioChannel();
    void start();
    void stop();
    void decode();
    void play();
    int getPCM();

    uint8_t *outBuffer;            //输出缓冲区
    int sampleRate;                //采样率

private:
    /**OpenSLES必要参数*/
    SLObjectItf engineObject;      //播放引擎
    SLEngineItf engineInterface;   //引擎接口
    SLObjectItf outputMixObject;   //混音器
    SLEnvironmentalReverbItf  outputMixInterface;      //混音器接口
    SLObjectItf player;            //播放器
    SLPlayItf playerInterface;     //播放器接口
    SLAndroidSimpleBufferQueueItf bufferQueueInterface;//播放队列接口
    /**重采样必要参数*/
    SwrContext *swrContext;        //重采样上下文
    int bufferSize;                //缓冲区大小
    int sampleSize;                //样本的大小
    int channelNum;                //通道数

    pthread_t pid_audio_decode;    //解码线程pid
    pthread_t pid_audio_play;      //播放线程pid

};

#endif //WYWONDERFULPLAYER_AUDIOCHANNEL_H
