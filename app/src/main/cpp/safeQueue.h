//
// Created by Acer on 2020/1/7.
//安全队列，用于读包、解码等数据的排队处理
//

#ifndef WYWONDERFULPLAYER_SAFEQUEUE_H
#define WYWONDERFULPLAYER_SAFEQUEUE_H

#include <queue>
#include <pthread.h>

using namespace std;

template <typename T>
class SafeQueue{
//自定义函数指针类型ReleaseCallback，由于T是未知类型，当需要释放时由外界处理
typedef void (*ReleaseCallback)(T *);
//同步函数指针，用于音视频同步
typedef void (*SyncCallback)(queue<T> &);
public:
    SafeQueue(){
        //动态初始化
        pthread_mutex_init(&mutex, nullptr);
        pthread_cond_init(&cond, nullptr);
    }
    ~SafeQueue(){
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    /**
     * 入队
     * @param value
     */
    void push(T value){
        pthread_mutex_lock(&mutex);
        if (work){
            q.push(value);
            pthread_cond_signal(&cond);//发送消息，告诉队列有数据了
        }else{
            if(releaseCallback){
                releaseCallback(&value);
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    /**
     * 出队
     * @param value
     * @return
     */
    int pop(T &value){
        int result = 0;
        pthread_mutex_lock(&mutex);
        while(work && empty()){
            pthread_cond_wait(&cond,&mutex);
        }
//        if(!empty()){
//            value = q.front();
//            q.pop();
//            result = 1;
//        }
        if(work && !empty()){
            value = q.front();
            q.pop();
            result = 1;
        }
        pthread_mutex_unlock(&mutex);
        return result;
    }

    /**
     * 清空队列
     */
    void clear(){
        pthread_mutex_lock(&mutex);
        int queueSize = size();
        for (int i = 0; i < queueSize; ++i) {
            T value = q.front();
            if(releaseCallback){
                releaseCallback(&value);
            }
            q.pop();
        }
        pthread_mutex_unlock(&mutex);
    }

    /**
     * 音视频同步函数
     * @return
     */
    void sync(){
        pthread_mutex_lock(&mutex);
        if(syncCallback){
            syncCallback(q);
        }
        pthread_mutex_unlock(&mutex);
    }

    void setWork(int state){
        pthread_mutex_lock(&mutex);
        work = state;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    int empty(){
        return q.empty();
    }

    int size(){
        return q.size();
    }

    void setReleaseCallback(ReleaseCallback releaseCallback){
        this->releaseCallback = releaseCallback;
    }
    void setSyncCallback(SyncCallback syncCallback){
        this->syncCallback = syncCallback;
    }

private:
    queue<T> q;
    int work = 0;                             //标记队列是否在工作状态
    pthread_mutex_t mutex;                    //互斥锁
    pthread_cond_t cond;                      //条件变量
    ReleaseCallback releaseCallback;//内存释放回掉接口
    SyncCallback syncCallback;      //音视频同步回掉接口
};

#endif //WYWONDERFULPLAYER_SAFEQUEUE_H
