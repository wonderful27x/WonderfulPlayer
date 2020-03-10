//
// Created by Acer on 2019/12/31.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int streamIndex,AVCodecContext *codecContext,AVRational time_base) : BaseChannel(streamIndex,codecContext,time_base) {
    LOGE("创建audioChannel...");
    sampleRate = 44100;                                                 //44100hz这是应用最广泛的，兼容性好
    sampleSize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);            //动态计算样本的大小
    channelNum = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);//动态计算通道数
    bufferSize = sampleRate * sampleSize * channelNum;                  //计算缓冲区大小
    outBuffer = static_cast<uint8_t *>(malloc(bufferSize));             //为输出缓冲区申请内存
    memset(outBuffer,0,bufferSize);                                     //清空缓冲区
    //重采样上下文,申请内存并初始化参数
    swrContext = swr_alloc_set_opts(nullptr,AV_CH_LAYOUT_STEREO,AV_SAMPLE_FMT_S16,sampleRate,
            codecContext->channel_layout,codecContext->sample_fmt,codecContext->sample_rate,
            0, nullptr);
    //初始化重采样上下文
    swr_init(swrContext);
    LOGE("创建audioChannel成功！");
}

void* audioDecodeTask(void *p){
    AudioChannel *audioChannel = static_cast<AudioChannel *>(p);
    audioChannel->decode();
    return nullptr;
}
//从压缩包队列读取数据包解码成PCM原始数据并放入PCM原始数据队列
void AudioChannel::decode() {
    LOGE("decode...");
    AVPacket *packet = nullptr;
    int result = 0;
    while(playState != STOP){

        //控制播放状态
        if(playState == PAUSE && !ENABLE_PAUSE_CACHE){
            av_usleep(PAUSE_SLEEP_TIME * 1000);//单位是微秒，乘以1000转换成毫秒
            continue;
        }

        /**
         * 控制音频frame队列最大值，超过最大值则让线程休眠一段时间，否则队列一直增长将会导致内存溢出
         */
        LOGD("audio_frame_size:%d",frameQueue.size());
        if(frameQueue.size() >= A_FRAME_QUEUE_SIZE){
            av_usleep(A_F_SLEEP_TIME * 1000);//单位是微秒，乘以1000转换成毫秒
            continue;
        }

        result = packetQueue.pop(packet);
        if (playState == STOP){
            break;
        }
        if (!result){
            //TODO need test
            releasePacket(&packet);
            continue;
        }
        result = avcodec_send_packet(codecContext,packet);
        if (result == AVERROR(EAGAIN)){
            //TODO need test
            releasePacket(&packet);
            continue;
        } else if(result != 0){
            break;
        }
        AVFrame *frame = av_frame_alloc();
        result = avcodec_receive_frame(codecContext,frame);
        if (result == AVERROR(EAGAIN)){
            releaseFrame(&frame);
            //TODO need test
            releasePacket(&packet);
            continue;
        } else if(result != 0){
            break;
        }
        frameQueue.push(frame);
        //TODO need test
        releasePacket(&packet);
        LOGE("解码音频成功！");
    }
    releasePacket(&packet);
    LOGD("audio decode stop!");
}

void* audioPlayTask(void *p){
    AudioChannel *audioChannel = static_cast<AudioChannel *>(p);
    audioChannel->play();
    return nullptr;
}

//接口回调函数,this callback handler is called every time a buffer finishes recording
void playerCallback(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *p){
    AudioChannel *audioChannel = static_cast<AudioChannel *>(p);
    int pcmSize = audioChannel->getPCM();
    if (pcmSize >0){
        (*bufferQueueItf)->Enqueue(bufferQueueItf,audioChannel->outBuffer,pcmSize);
    }
}

//初始化音频播放的重要参数并设置播放回调函数
void AudioChannel::play() {
    /**
     * 1.创建引擎并初始化引擎接口
     */
    SLresult result;
    //创建引擎对象
    result = slCreateEngine(&engineObject,0, nullptr,0, nullptr, nullptr);
    if(result != SL_RESULT_SUCCESS){
        //TODO
        LOGE("创建引擎失败！");
        return;
    }
    //初始化引擎
    result = (*engineObject)->Realize(engineObject,SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS){
        //TODO
        LOGE("初始化擎失败！");
        return;
    }
    //初始化引擎接口
    result = (*engineObject)->GetInterface(engineObject,SL_IID_ENGINE,&engineInterface);
    if(result != SL_RESULT_SUCCESS){
        //TODO
        LOGE("初始化引擎接口失败！");
        return;
    }
    /**
     * 2.设置混音器
     */
    //创建混音器
    result = (*engineInterface)->CreateOutputMix(engineInterface,&outputMixObject,0, nullptr,nullptr);
    if(result != SL_RESULT_SUCCESS){
        //TODO
        LOGE("创建混音器失败！");
        return;
    }
    //初始化混音器
    result = (*outputMixObject)->Realize(outputMixObject,SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS){
        //TODO
        LOGE("初始化混音器失败！");
        return;
    }
/*
    //不启用混响可以不用获取混音器接口
    // 获得混音器接口
    //设置混响 ： 默认。
    //SL_I3DL2_ENVIRONMENT_PRESET_ROOM: 室内
    //SL_I3DL2_ENVIRONMENT_PRESET_AUDITORIUM : 礼堂
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,&outputMixInterface);
    if (SL_RESULT_SUCCESS == result) {
        const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
        (*outputMixInterface)->SetEnvironmentalReverbProperties(outputMixInterface, &settings);
    }
*/

    /**
     * 3.创建播放器
     */
    //配置输入声音信息
    //创建2个buffer缓冲类型的队列
    SLDataLocator_AndroidSimpleBufferQueue queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
    //这是PCM数据格式
    SLDataFormat_PCM pcmData = {
        SL_DATAFORMAT_PCM,                             //PCM数据格式
        2,                                             //声道数：双声道
        SL_SAMPLINGRATE_44_1,                          //44100采样率
        SL_PCMSAMPLEFORMAT_FIXED_16,                   //16bit采用格式
        SL_PCMSAMPLEFORMAT_FIXED_16,                   //16bit数据大小
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//左右声道
        SL_BYTEORDER_LITTLEENDIAN                      //小端模式
    };
    //数据源，将上面配置设置到数据源中
    SLDataSource dataSource = {&queue,&pcmData};
    //配置输出音轨
    //设置混音器
    SLDataLocator_OutputMix locatorOutputMix = {SL_DATALOCATOR_OUTPUTMIX,outputMixObject};
    SLDataSink dataSink = {&locatorOutputMix, nullptr};
    //需要的接口,操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    //创建播放器
    result = (*engineInterface)->CreateAudioPlayer(engineInterface,&player,&dataSource,&dataSink,1,ids,req);
    if(result != SL_RESULT_SUCCESS){
        //TODO
        LOGE("创建播放器失败！");
        return;
    }
    //初始化播放器
    result = (*player)->Realize(player,SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS){
        //TODO
        LOGE("初始化播放器失败！");
        return;
    }
    //初始化播放器接口
    result = (*player)->GetInterface(player,SL_IID_PLAY,&playerInterface);
    if(result != SL_RESULT_SUCCESS){
        //TODO
        LOGE("初始化播放器接口失败！");
        return;
    }
    /**
     * 4.设置播放回调函数
     */
     //获取播放队列接口
    (*player)->GetInterface(player,SL_IID_BUFFERQUEUE,&bufferQueueInterface);
     //设置回调
    (*bufferQueueInterface)->RegisterCallback(bufferQueueInterface,playerCallback,this);
    /**
     * 5.设置播放器状态为播放状态
     */
    (*playerInterface)->SetPlayState(playerInterface,SL_PLAYSTATE_PLAYING);
    /**
     * 6.手动激活回掉函数
     */
     playerCallback(bufferQueueInterface,this);
     LOGE("初始化OPENSLES成功！");
}

/**
 * 获取重采样PCM数据,这个函数是会被重复调用的，每次取一帧数据就返回
 * @return 返回一帧PCM数据的大小
 */
int AudioChannel::getPCM() {
    int result = 0;
    int pcmSize = 0;
    AVFrame *frame = nullptr;
    while(playState != STOP){

        //控制播放状态
        if(playState == PAUSE){
            av_usleep(PAUSE_SLEEP_TIME * 1000);//单位是微秒，乘以1000转换成毫秒
            continue;
        }

        result = frameQueue.pop(frame);
        if (playState == STOP){
            //TODO
            break;
        }
        if (!result){
            //TODO need test
            releaseFrame(&frame);
            continue;
        }
        //计算输出样本数，算法是，输出样本数 =（输入样本数）*（输入采样率）/（输出采用率）
        int64_t outSampleNum = av_rescale_rnd(
                swr_get_delay(swrContext,frame->sample_rate) + frame->nb_samples,//输入样本数加一个尾巴
                frame->sample_rate, //输入采用率
                sampleRate,         //输出采用率
                AV_ROUND_UP         //向上取整
                );
        //重采样，返回每个声道样本数
        int singleSampleNum = swr_convert(
                swrContext,
                &outBuffer,
                (int)(outSampleNum),
                (const uint8_t **)(frame->data),
                frame->nb_samples);
        //计算输出pcm数据大小,每个声道样本数 * 每个样本大小 * 声道数
        pcmSize = singleSampleNum * sampleSize * channelNum;
        //每帧音频的时间,这稍微做一下转换，因为他有自己的一套时间单位
        play_time = frame->best_effort_timestamp * av_q2d(time_base);
//        if(jniCallback && duration != 0){
//            jniCallback->progressUpdate(THREAD_CHILD,play_time);
//        }
        if(jniCallback){
            jniCallback->progressUpdate(THREAD_CHILD,play_time);
        }
        break;
    }
    releaseFrame(&frame);
    return pcmSize;
}

//解码、播放
void AudioChannel::start() {
    LOGE("开始播放音频！");
//    isPlaying = 1;
    playState = PLAYING;
    packetQueue.setWork(1);
    frameQueue.setWork(1);
    pthread_create(&pid_audio_decode, nullptr,audioDecodeTask,this);
    pthread_create(&pid_audio_play, nullptr,audioPlayTask,this);
    LOGE("播放ok！");
}

//停止播放
void AudioChannel::stop() {
    LOGD("stop-audioChannel_P_size:%d",packetQueue.size());
    LOGD("stop-audioChannel_F_size:%d",frameQueue.size());
    playState = STOP;
    packetQueue.setWork(0);
    frameQueue.setWork(0);
    pthread_join(pid_audio_decode, nullptr);
    pthread_join(pid_audio_play, nullptr);
    packetQueue.clear();
    frameQueue.clear();

    //设置停止状态
    if (playerInterface) {
        (*playerInterface)->SetPlayState(playerInterface, SL_PLAYSTATE_STOPPED);
        playerInterface = nullptr;
    }
   //销毁播放器
    if (player) {
        (*player)->Destroy(player);
        player = nullptr;
        bufferQueueInterface = nullptr;
    }
    //销毁混音器
    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = nullptr;
    }
    //销毁引擎
    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = nullptr;
        engineInterface = nullptr;
    }

    if(codecContext){
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
    if(swrContext){
        swr_free(&swrContext);
        swrContext = nullptr;
    }
    DELETE(outBuffer);

    if(destroyCall){
        destroyCall(streamIndex);
    }
}

//析构
AudioChannel::~AudioChannel() {
    LOGD("AudioChannel 析构！");
}
