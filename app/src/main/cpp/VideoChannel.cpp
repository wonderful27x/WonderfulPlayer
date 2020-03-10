//
// Created by Acer on 2019/12/31.
//

#include "VideoChannel.h"

//丢弃压缩包
void dropPacket(queue<AVPacket *> &queue){
    while(!queue.empty()){
        AVPacket *packet = queue.front();
        //如果不是关键帧
        if (packet->flags != AV_PKT_FLAG_KEY){
            BaseChannel::releasePacket(&packet);
            queue.pop();
        }else{
            break;
        }
    }
}
//丢弃解密帧
void dropFrame(queue<AVFrame *> &queue){
    if(!queue.empty()){
        AVFrame *frame = queue.front();
        BaseChannel::releaseFrame(&frame);
        queue.pop();
    }
}

VideoChannel::VideoChannel(int streamIndex,AVCodecContext *codecContext,AVRational time_base,int fps) : BaseChannel(streamIndex,codecContext,time_base) {
    this->fps = fps;
    packetQueue.setSyncCallback(dropPacket);
    frameQueue.setSyncCallback(dropFrame);
}

void* videoDecodeTask(void *p){
    VideoChannel *videoChannel = static_cast<VideoChannel*>(p);
    videoChannel->decode();
    return nullptr;
}

//循环从队列中取出数据包进行解码，然后放入yuv原始数据队列
void VideoChannel::decode() {
    AVPacket *packet = nullptr;
    int result = 0;
    while(playState != STOP){

        //控制播放状态
        if(playState == PAUSE && !ENABLE_PAUSE_CACHE){
            av_usleep(PAUSE_SLEEP_TIME * 1000);//单位是微秒，乘以1000转换成毫秒
            continue;
        }

        /**
         * 控制视频frame队列最大值，超过最大值则让线程休眠一段时间，否则队列一直增长将会导致内存溢出
         */
        LOGD("video_frame_size:%d",frameQueue.size());
        if(frameQueue.size() >= V_FRAME_QUEUE_SIZE){
            av_usleep(V_F_SLEEP_TIME * 1000);//单位是微秒，乘以1000转换成毫秒
            continue;
        }

        result = packetQueue.pop(packet);
        if(playState == STOP){
            break;
        }
        if(!result){
            //TODO need test
            releasePacket(&packet);
            continue;
        }
        result = avcodec_send_packet(codecContext,packet);
        if(result == AVERROR(EAGAIN)){
            //TODO need test
            releasePacket(&packet);
            continue;
        } else if(result != 0){
            break;
        }
        AVFrame *frame = av_frame_alloc();
        result = avcodec_receive_frame(codecContext,frame);
        if(result == AVERROR(EAGAIN)){
            releaseFrame(&frame);
            //TODO need test
            releasePacket(&packet);
            continue;
        } else if(result != 0){
            break;
        }

        frameQueue.push(frame);
        //TODO need test
        // 所有TODO need test 标记的地方都是为解决内存泄漏的，
        // 但是这样写有可能造成其他问题没有全面测试过
        releasePacket(&packet);
    }
    releasePacket(&packet);
    LOGD("video decode stop!");
}

void* videoPlayTask(void* p){
    VideoChannel *videoChannel = static_cast<VideoChannel*>(p);
    videoChannel->play();
    return nullptr;
}

//循环从队列中取出数据并转换为rgba并绘制到surface
void VideoChannel::play() {
    AVFrame *frame = nullptr;
    uint8_t *dst_data[4];
    int lineSize[4];
    int result = 0;
    SwsContext *swsContext = sws_getContext(codecContext->width,codecContext->height,codecContext->pix_fmt,
                                            codecContext->width,codecContext->height,AV_PIX_FMT_RGBA,
                                            SWS_BILINEAR, nullptr, nullptr, nullptr);
    av_image_alloc(dst_data,lineSize,codecContext->width,codecContext->height,AV_PIX_FMT_RGBA,1);
    while (playState != STOP){

        //控制播放状态
        if(playState == PAUSE){
            av_usleep(PAUSE_SLEEP_TIME * 1000);//单位是微秒，乘以1000转换成毫秒
            continue;
        }

        result = frameQueue.pop(frame);
        if (playState == STOP){
            break;
        }
        if (!result){
            //TODO need test
            releaseFrame(&frame);
            continue;
        }
        //开始转换YUV->RGBA
        sws_scale(swsContext,frame->data,frame->linesize,0,codecContext->height,dst_data,lineSize);

        /**
         * 渲染前处理音视频同步，这里是以音频时间为基准，这种效果会更好些
         */

        play_time = frame->best_effort_timestamp * av_q2d(time_base); //没帧视频时间

        double frameTime = 1.0/fps;                       //根据fps获取每一帧的播放时间
        double extraDelay = frame->repeat_pict / (2*fps); //获取额外延时时间，算法固定
        double realDelay = frameTime + extraDelay;

        //没有音频则按照原帧率进行播放
        if(!audioChannel){
            av_usleep(realDelay * 1000000);
//            if(jniCallback && duration != 0){
//                jniCallback->progressUpdate(THREAD_CHILD,play_time);
//            }
            if(jniCallback){
                jniCallback->progressUpdate(THREAD_CHILD,play_time);
            }
        }
        else{
            double audioTime = audioChannel->play_time;
            double timeDiff = play_time - audioTime;
            //视频快了，则等待音频
            if (timeDiff >0){
                //当差得太大时让音频慢慢追
                if (timeDiff > V_FAST_THRESHOLD){
                    av_usleep(realDelay * CHASE_UP_RATE * 1000000);
                }
                else{
                    av_usleep((realDelay + timeDiff) * 1000000);
                }
            }
            //音频快了，则适当丢弃一些帧
            else if(timeDiff <0){
                if (fabs(timeDiff) > A_SLOW_THRESHOLD){
                    frameQueue.sync();
//                    packetQueue.sync();
                    releaseFrame(&frame);
                    continue;
                }
            }
            //完美同步
            else{

            }
        }

        //绘制RGBA数据
        if(renderCallback){
            renderCallback(dst_data[0],codecContext->width,codecContext->height,lineSize[0]);
        }
        releaseFrame(&frame);
    }
    releaseFrame(&frame);
    av_free(dst_data[0]);
    sws_freeContext(swsContext);
    LOGD("video play stop!");
}

/**
 * 解码和播放
 */
void VideoChannel::start() {
//    isPlaying = 1;
    playState = PLAYING;
    packetQueue.setWork(1);
    frameQueue.setWork(1);
    pthread_create(&pid_video_decode, nullptr,videoDecodeTask,this);
    pthread_create(&pid_video_play, nullptr,videoPlayTask,this);
}

void VideoChannel::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}


void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audioChannel = audioChannel;
}

void VideoChannel::stop() {
    LOGD("stop-videoChannel_P_size:%d",packetQueue.size());
    LOGD("stop-videoChannel_F_size:%d",frameQueue.size());
    playState = STOP;
    packetQueue.setWork(0);
    frameQueue.setWork(0);
    pthread_join(pid_video_decode, nullptr);
    pthread_join(pid_video_play, nullptr);
    packetQueue.clear();
    frameQueue.clear();
    if(codecContext){
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
    if(destroyCall){
        destroyCall(streamIndex);
    }
}

VideoChannel::~VideoChannel() {
    LOGD("VideoChannel 析构！");
}


