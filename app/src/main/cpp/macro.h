//
// Created by Acer on 2019/12/31.
//

#ifndef WYWONDERFULPLAYER_MACRO_H
#define WYWONDERFULPLAYER_MACRO_H

#define THREAD_MAIN 0             //主线程
#define THREAD_CHILD 1            //子线程

#define V_PACKET_QUEUE_SIZE 100  //视频packet队列最大值
#define A_PACKET_QUEUE_SIZE 100  //音频packet队列最大值
#define V_FRAME_QUEUE_SIZE 100   //视频frame队列最大值
#define A_FRAME_QUEUE_SIZE 100   //音频frame队列最大值
#define V_P_SLEEP_TIME 100       //视频packet队列超过最大值休眠时间，毫秒
#define A_P_SLEEP_TIME 100       //音频packet队列超过最大值休眠时间，毫秒
#define V_F_SLEEP_TIME 10        //视频frame队列超过最大值休眠时间，毫秒
#define A_F_SLEEP_TIME 10        //音频frame队列超过最大值休眠时间，毫秒

#define PAUSE_SLEEP_TIME 10      //暂停休眠时间，毫秒
#define ENABLE_PAUSE_CACHE 1     //是否开启暂停缓存，暂停时继续解码，直到缓存队列充满

#define V_FAST_THRESHOLD 1       //视频比音频快的阈值，秒
#define CHASE_UP_RATE 2          //追赶控制比例
#define A_SLOW_THRESHOLD 0.05    //音频比视频快的阈值，秒

#define DELETE(object) if(object){delete object;object=0;}//释放内存

#endif //WYWONDERFULPLAYER_MACRO_H
