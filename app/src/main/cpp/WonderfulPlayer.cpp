//
// Created by Acer on 2019/12/31.
//

#include <cstring>
#include <pthread.h>
#include <thread>
#include "WonderfulPlayer.h"
#include "macro.h"
#include "androidlog.h"

extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

void playerDestroyCall(int type);

WonderfulPlayer* WonderfulPlayer::wonderfulPlayer = nullptr;
WonderfulPlayer::WonderfulPlayer() {}
WonderfulPlayer::WonderfulPlayer(const char *sourcePath){}

void WonderfulPlayer::init(const char *sourcePath) {
    //初始化播放地址，因为sourcePath传递进来后可能会被释放，形成指针悬空
    //所以不能直接赋值，需要申请内存自己管理
    this->sourcePath = new char[strlen(sourcePath) + 1];
    strcpy(this->sourcePath,sourcePath);
    pthread_mutex_init(&formatContextMutex, nullptr);
    pthread_mutex_init(&destroyMutex, nullptr);
}

WonderfulPlayer* WonderfulPlayer::getInstance() {
    if(!wonderfulPlayer){
        wonderfulPlayer = new WonderfulPlayer();
    }
    return wonderfulPlayer;
}

//线程运行的目标函数
void* prepareTask(void *player){
    WonderfulPlayer *wonderfulPlayer = static_cast<WonderfulPlayer*>(player);
    wonderfulPlayer->prepareExecute();
    return nullptr;
}

void WonderfulPlayer::prepare() {
    //由于初始化可能是耗时操作，开启线程来处理
    pthread_create(&pid_prepare,nullptr,prepareTask,this);
}

//初始化FFMPEG
void WonderfulPlayer::prepareExecute() {
    /**
     * 1.创建全局上下文，并打开音视频文件，获取header信息
     */
    avformat_network_init();//初始化网络模块，因为ffmpeg支持本地和网络数据播放
    formatContext = avformat_alloc_context();//为全局上下文申请内存空间
    AVDictionary *dictionary = nullptr;                             //创建一个字典，用于设置一些参数，如超时时长
    av_dict_set(&dictionary,"timeout","5000000",0);
    int result = avformat_open_input(&formatContext,sourcePath,nullptr,&dictionary);//打开文件，获取header信息
    av_dict_free(&dictionary);
    if(result){
        //TODO 打开文件失败
        if(jniCallback){
            jniCallback->error(THREAD_CHILD,OPEN_FAIL);
        }
        LOGE("打开文件失败，sourcePath：%s",sourcePath);
        return;
    }

    /**
     * 2.获取流信息
     */
    result = avformat_find_stream_info(formatContext,nullptr);//获取流信息
    if(result <0 ){
        //TODO 获取流信息失败
        LOGE("获取流信息失败");
        return;
    }

    //获取总时长
    duration = formatContext->duration / AV_TIME_BASE;

    /**
     * 3.遍历流，根据流获取解码的关键参数并进行分流处理，流有可能有多个，如视频流和音频流
     */
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        AVStream *avStream = formatContext->streams[i];                    //获取媒体流
        AVCodecParameters *codecParameters = avStream->codecpar;           //从流中获取解密器参数
        AVCodec *avCodec = avcodec_find_decoder(codecParameters->codec_id);//根据解密参数id获取解密器
        if(!avCodec){
            //TODO 获取解密器失败
            LOGE("获取解密器失败");
            return;
        }
        AVCodecContext *codecContext = avcodec_alloc_context3(avCodec);     //创建解码器上下文
        if (!codecContext){
            //TODO 获取解码器上下文失败
            LOGE("获取解码器上下文失败");
            return;
        }
        result = avcodec_parameters_to_context(codecContext,codecParameters);//设置解码器上下文参数
        if (result <0 ){
            //TODO 设置解码器上下文参数失败
            LOGE("设置解码器上下文参数失败");
            return;
        }
        result = avcodec_open2(codecContext,avCodec,nullptr);                      //打开解码器初始化解密器上下文
        if(result){
            //TODO 打开解码器失败
            LOGE("打开解码器失败");
            return;
        }

        /**
         * 根据解密器参数判断流类型，进行分流处理
         */
         AVRational time_base = avStream->time_base;      //从流中获取时间基，ffmpeg的一个时间单位，后面计算播放时间需要用到
        if(codecParameters->codec_type == AVMEDIA_TYPE_AUDIO){
            audioStreamIndex = i;
            audioChannel = new AudioChannel(i,codecContext,time_base);
            audioChannel->duration = duration;
            audioChannel->setJniCallback(jniCallback);
            audioChannel->setDestroyCall(playerDestroyCall);
//            audioChannel->setDestroyCall(nullptr);
        } else if(codecParameters->codec_type == AVMEDIA_TYPE_VIDEO){
            //如果标记是附加图，则过滤掉，因为这不是视频播放内容，而可能是封面
            if(avStream->disposition & AV_DISPOSITION_ATTACHED_PIC){
                continue;
            }
            videoStreamIndex = i;
            AVRational frame_rate = avStream->avg_frame_rate;//获取平均帧率
            int fps = av_q2d(frame_rate);                    //转换为fps
            videoChannel = new VideoChannel(i,codecContext,time_base,fps);
            videoChannel->duration = duration;
            videoChannel->setRenderCallback(renderCallback);
            videoChannel->setJniCallback(jniCallback);
            videoChannel->setDestroyCall(playerDestroyCall);
//            videoChannel->setDestroyCall(nullptr);
        }
    }
    if (!audioChannel && !videoChannel){
        //TODO 未获取到任何流
        LOGE("未获取到任何流");
        return;
    }
    if(jniCallback){
        jniCallback->prepareSuccess(THREAD_CHILD);
    }
}

//线程运行的目标函数
void* startTask(void *player){
    WonderfulPlayer *wonderfulPlayer = static_cast<WonderfulPlayer*>(player);
    wonderfulPlayer->startExecute();
    return nullptr;
}

void WonderfulPlayer::start() {
//    isPlaying = 1;                                    //设置播放状态
    playState = PLAYING;                                //设置播放状态
    if(videoChannel){
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->start();                        //视频通道开始处理
        LOGE("启动videoChannel！");
    }

    if (audioChannel){
        audioChannel->start();                         //音频通道开始处理
        LOGE("启动audioChannel！");
    }
    pthread_create(&pid_start,nullptr,startTask,this);//开启一个线程对音视频流进行处理，读取压缩包packet加入对应的队列中
}

//开启一个线程进行读包处理，循环读取数据包并放入队列中
void WonderfulPlayer::startExecute() {
    int result = 0;
    AVPacket *packet = nullptr;
    while(playState != STOP){

        //暂停状态
        if(playState == PAUSE && !ENABLE_PAUSE_CACHE){
            av_usleep(PAUSE_SLEEP_TIME * 1000);//单位是微秒，乘以1000转换成毫秒
            continue;
        }

        /**
         * 控制音视频packet队列最大值，超过最大值则让线程休眠一段时间，否则队列一直增长将会导致内存溢出
         * 然而奇怪的是注释后即便packet队列大小达到了7000多内存依然十分稳定，倒是进行控制后会造成小量的内存泄漏,
         * 难道这个packet并没有被读到内存中吗??? 到底是什么原因造成了continue后的内存泄漏???
         * 另一个值得注意的问题是：播放时每间隔15秒左右内存会被小量释放一次，具体进行Profiler查看，后来发现这是java层的gc
         * 并且这种内存波动造成了音频的卡顿，视频影响不大,然而实验证明不跑Profiler的情况下这种卡顿不会发生！！！
         *
         * 所有的内存泄漏基本都解决了，TODO test中
         */

        if(videoChannel){
            LOGD("video_packet_size:%d",videoChannel->packetQueue.size());
        }
        if(audioChannel){
            LOGD("audio_packet_size:%d",audioChannel->packetQueue.size());
        }

        if(videoChannel && videoChannel->packetQueue.size() >= V_PACKET_QUEUE_SIZE){
            av_usleep(V_P_SLEEP_TIME * 1000);//单位是微秒，乘以1000转换成毫秒
//            this_thread::sleep_for(chrono::milliseconds(V_P_SLEEP_TIME));
            continue;
        }
        if(audioChannel && audioChannel->packetQueue.size() >= A_PACKET_QUEUE_SIZE){
            av_usleep(A_P_SLEEP_TIME * 1000);//单位是微秒，乘以1000转换成毫秒
//            this_thread::sleep_for(chrono::milliseconds(V_P_SLEEP_TIME));
            continue;
        }

        packet = av_packet_alloc();

        pthread_mutex_lock(&formatContextMutex);
        result = av_read_frame(formatContext,packet);
        pthread_mutex_unlock(&formatContextMutex);

        if(result == 0){
            //视频流
            if (videoChannel && packet->stream_index == videoChannel->streamIndex){
                videoChannel->packetQueue.push(packet);
            }
            //音频流
            else if(audioChannel && packet->stream_index == audioChannel->streamIndex){
                audioChannel->packetQueue.push(packet);
            } else{
                if (packet){
                    av_packet_free(&packet);
                    packet = nullptr;
                }
            }
        } else if(result == AVERROR_EOF){
            //TODO 读到结尾了 这里必须break，否则将会造成非常严重的内存泄漏
            break;
        } else{
            //TODO 错误
            break;
        }
    }
    if (packet){
        av_packet_free(&packet);
    }
    LOGD("player stop!");
}

void WonderfulPlayer::setJniCallback(JniCallback *jniCallback) {
    this->jniCallback = jniCallback;
}

void WonderfulPlayer::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}

double WonderfulPlayer::getDuration() {
    return duration;
}

//修改播放进度
void WonderfulPlayer::seek(double progress) {

    if(progress <0 || progress >duration)return;
    if(!videoChannel && !audioChannel)return;
    if(!formatContext)return;

    pthread_mutex_lock(&formatContextMutex);
    int result = av_seek_frame(formatContext, -1,static_cast<int64_t>(progress * duration * AV_TIME_BASE), AVSEEK_FLAG_BACKWARD);
    if (result <0){
        //TODO
        pthread_mutex_unlock(&formatContextMutex);
        return;
    }

    audioChannel->packetQueue.setWork(0);
    audioChannel->frameQueue.setWork(0);
    videoChannel->packetQueue.setWork(0);
    videoChannel->frameQueue.setWork(0);

    audioChannel->packetQueue.clear();
    audioChannel->frameQueue.clear();
    videoChannel->packetQueue.clear();
    videoChannel->frameQueue.clear();

    audioChannel->packetQueue.setWork(1);
    audioChannel->frameQueue.setWork(1);
    videoChannel->packetQueue.setWork(1);
    videoChannel->frameQueue.setWork(1);

    pthread_mutex_unlock(&formatContextMutex);
}

//控制播放状态
void WonderfulPlayer::playControl(PlayState state) {
    switch (state){
        case PLAYING:
            if(playState == STOP){
                //TODO
            } else if(playState == PLAYING){

            } else if(playState == PAUSE){
                playState = PLAYING;
                audioChannel->playControl(PLAYING);
                videoChannel->playControl(PLAYING);
            }
            break;
        case PAUSE:
            if(playState == STOP){

            } else if(playState == PLAYING){
                playState = PAUSE;
                audioChannel->playControl(PAUSE);
                videoChannel->playControl(PAUSE);
            } else if(playState == PAUSE){

            }
            break;
        default:
            break;
    }
}

//这里释放有一个思想就是使用join让线程先跑完
void *stopTask(void *player){
    WonderfulPlayer *wonderfulPlayer = static_cast<WonderfulPlayer*>(player);
    wonderfulPlayer->playState = STOP;
    //如果不开子线程，这里会引发ANR
    pthread_join(wonderfulPlayer->pid_prepare, nullptr);

    //再次切断接口
    if(wonderfulPlayer->audioChannel){
        wonderfulPlayer->audioChannel->jniCallback = nullptr;
    }
    if(wonderfulPlayer->videoChannel){
        wonderfulPlayer->videoChannel->jniCallback = nullptr;
        wonderfulPlayer->videoChannel->setRenderCallback(nullptr);
    }

    pthread_join(wonderfulPlayer->pid_start, nullptr);

    if(wonderfulPlayer->audioChannel){
        wonderfulPlayer->audioChannel->stop();
    }
    if(wonderfulPlayer->videoChannel){
        wonderfulPlayer->videoChannel->stop();
    }

//    //TODO 第二种方式，当setDestroyCall等于null时
//    if(wonderfulPlayer->formatContext){
//        avformat_close_input(&wonderfulPlayer->formatContext);
//        avformat_free_context(wonderfulPlayer->formatContext);
//        wonderfulPlayer->formatContext = nullptr;
//    }
//    DELETE(wonderfulPlayer->audioChannel);
//    DELETE(wonderfulPlayer->videoChannel);

    return nullptr;
}

void WonderfulPlayer::stop() {
    //切断上层调用接口
    jniCallback = nullptr;
    renderCallback = nullptr;
    if(audioChannel){
        audioChannel->jniCallback = nullptr;
    }
    if(videoChannel){
        videoChannel->setAudioChannel(nullptr);
        videoChannel->jniCallback = nullptr;
        videoChannel->setRenderCallback(nullptr);
    }
    //开启一个线程停止
    pthread_create(&pid_stop, nullptr,stopTask,this);
}

WonderfulPlayer::~WonderfulPlayer() {
    DELETE(sourcePath);
    pthread_mutex_destroy(&formatContextMutex);
    pthread_mutex_destroy(&destroyMutex);
    LOGD("WonderfulPlayer 析构！");
}

//外部的释放回掉，设置给WonderfulPlayer
void WonderfulPlayer::setDestroyCall(void (*destroyCall)(void)) {
    this->destroyCall = destroyCall;
}

//内部的释放回掉，将会设置给VideoChannel和AudioChannel,
//当这个方法被调用说明VideoChannel和AudioChannel内存释放完成，
//接下来就是释放自己的内存，当自己的内存释放完毕再回到最外层，
//这是一种由内向外的内存释放机制
//不过这种机制似乎有问题，存在自己销毁自己的情景，有待研究
void playerDestroyCall(int type) {
    WonderfulPlayer *wonderfulPlayer = WonderfulPlayer::getInstance();
    wonderfulPlayer->destroy(type);
}

void WonderfulPlayer::destroy(int type) {
    LOGD("destroy...");
    pthread_mutex_lock(&destroyMutex);
    //audioChannel资源释放完成
    if(type == audioStreamIndex){
        LOGD("destroy-audioChannel_P_size:%d",audioChannel->packetQueue.size());
        LOGD("destroy-audioChannel_F_size:%d",audioChannel->frameQueue.size());
        audioStreamIndex = -1;
        audioChannel->setDestroyCall(nullptr);
        DELETE(audioChannel);
        LOGD("destroy-audioChannel");
    }
    //videoChannel资源释放完成
    else if(type == videoStreamIndex){
        LOGD("destroy-videoChannel__P_size:%d",videoChannel->packetQueue.size());
        LOGD("destroy-videoChannel__F_size:%d", videoChannel->frameQueue.size());
        videoStreamIndex = -1;
        videoChannel->setDestroyCall(nullptr);
        DELETE(videoChannel);
        LOGD("destroy-videoChannel");
    }

    if(audioStreamIndex == -1 && videoStreamIndex == -1){
        if(formatContext){
            avformat_close_input(&formatContext);
            avformat_free_context(formatContext);
            formatContext = nullptr;
        }
        if(destroyCall){
            destroyCall();
        }
        LOGD("destroy-destroyCall");
    }
    pthread_mutex_unlock(&destroyMutex);
}
