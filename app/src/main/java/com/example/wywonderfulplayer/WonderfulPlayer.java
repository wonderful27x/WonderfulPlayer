package com.example.wywonderfulplayer;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

//音视频播放控制类，主要通过native调用控制音视频的播放
public class WonderfulPlayer implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("wonderful");
    }

    private JniCallback jniCallback;            //Jni回调接口
    private String dataSource;                  //播放地址
    private SurfaceHolder surfaceHolder;        //surfaceHolder
    private int duration = 0;                   //播放总时长
    private @PlayState int playState = PlayState.STOP;     //播放状态

    public WonderfulPlayer(String dataSource){
        this.dataSource = dataSource;
    }

    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    public String getDataSource() {
        return dataSource;
    }

    //准备播放
    public void prepare(){
        prepareNative(dataSource);
    }
    //开始播放
    public void start(){
        startNative();
    }
    //停止播放
    public void stop(){
        stopNative();
    }

    //初始化surface
    public void setSurfaceView(SurfaceView surfaceView){
        if (surfaceHolder != null){
            surfaceHolder.removeCallback(this);
        }
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
    }

    //获取总时长
    public void getDurationForNative(){
        this.duration = getDurationNative();
    }

    public int getDuration(){
        return duration;
    }

    //画布创建好回调
    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    //画布发生改变是回调
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        setSurfaceNative(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    interface JniCallback{
        public void prepareSuccess();               //准备成功
        public void error(int code);                //错误信息
        public void progressUpdate(double progress);//播放进度条更新
    }

    //播放准备工作完成的回调方法，由native反射调用
    public void prepareSuccess(){
        if (jniCallback != null){
            jniCallback.prepareSuccess();
        }
    }

    //解析、播放错误回调，由native反射调用
    public void error(@ErrorType int code){
        if (jniCallback != null){
            jniCallback.error(code);
        }
    }

    //播放进度回调，由native反射调用
    public void progressUpdate(double progress){
        if (jniCallback != null){
            jniCallback.progressUpdate(progress);
        }
    }
    //改变播放进度
    public void seek(double progress){
        seekNative(progress);
    }

    public void setJniCallback(JniCallback jniCallback){
        this.jniCallback = jniCallback;
    }

    public void setPlayState(@PlayState int playState) {
        this.playState = playState;
    }

    public @PlayState int getPlayState() {
        return playState;
    }

    public void playControl(@PlayState int state){
        this.playState = state;
        playControlNative(state);
    }

    /**
     * native 方法
     * @param dataSource
     */
    private native void prepareNative(String dataSource);
    private native void startNative();
    private native void stopNative();
    private native void setSurfaceNative(Surface surface);
    private native int getDurationNative();
    private native void seekNative(double progress);
    private native void playControlNative(@PlayState int state);
}
