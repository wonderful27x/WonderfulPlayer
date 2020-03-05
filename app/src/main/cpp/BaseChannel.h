//
// Created by Acer on 2020/1/7.
//

#ifndef WYWONDERFULPLAYER_BASECHANNEL_H
#define WYWONDERFULPLAYER_BASECHANNEL_H

#include "safeQueue.h"
#include "JniCallback.h"
#include "wonderfulEnum.h"

extern "C"{
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
};

//音视频基类通道，提取公用信息
class BaseChannel{

public:
    BaseChannel(int streamIndex,AVCodecContext *codecContext,AVRational time_base){
        this->streamIndex = streamIndex;
        this->codecContext = codecContext;
        this->time_base = time_base;
        packetQueue.setReleaseCallback(releasePacket);
        frameQueue.setReleaseCallback(releaseFrame);
    }

    virtual ~BaseChannel(){
        packetQueue.setWork(0);
        frameQueue.setWork(0);
        packetQueue.clear();
        frameQueue.clear();
    }

    //释放packet
    static void releasePacket(AVPacket **packet){
        if (*packet){
            av_packet_free(packet);
            *packet = nullptr;
        }
    }

    //释放frame
    static void releaseFrame(AVFrame **frame){
        if(*frame){
            av_frame_free(frame);
            *frame = nullptr;
        }
    }

    void setJniCallback(JniCallback *jniCallback){
        this->jniCallback = jniCallback;
    }

    void setDestroyCall(void (*destroyCall)(int type)){
        this->destroyCall = destroyCall;
    }

    //控制播放状态
    void playControl(PlayState state){
        switch (state){
            case PLAYING:
                if(playState == STOP){
                    //TODO
                } else if(playState == PLAYING){

                } else if(playState == PAUSE){
                    playState = PLAYING;
                }
                break;
            case PAUSE:
                if(playState == STOP){

                } else if(playState == PLAYING){
                    playState = PAUSE;
                } else if(playState == PAUSE){

                }
                break;
            default:
                break;
        }
    }

    SafeQueue<AVPacket *> packetQueue;   //压缩包队列
    SafeQueue<AVFrame *> frameQueue;     //原始数据队列
    AVCodecContext *codecContext;        //解码器上下文
    int streamIndex = -1;                //流索引，用于区分音视频流
//    int isPlaying = 0;                   //播放状态
    PlayState playState = STOP;          //播放状态
    AVRational time_base ;               //时间基，相当于一个时间单位
    double play_time;                    //播放时间
    double duration = 0;                 //总时长
    JniCallback *jniCallback = nullptr;  //jni回调
    void (*destroyCall)(int type);        //内存释放回掉

};

#endif //WYWONDERFULPLAYER_BASECHANNEL_H
