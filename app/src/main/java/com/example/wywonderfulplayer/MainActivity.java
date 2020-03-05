package com.example.wywonderfulplayer;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.Manifest;
import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Debug;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;
import java.io.File;

public class MainActivity extends AppCompatActivity implements SeekBar.OnSeekBarChangeListener,View.OnClickListener{

    private static final String TAG = "MainActivity";
    private SurfaceView surfaceView;
    private WonderfulPlayer wonderfulPlayer;
    private SeekBar seekBar;
    private Button control;
    private Button stop;
    private Button localStart;
    private Button netStart;
    private TextView time;
    private EditText name;
    private LinearLayout seekLayout;
    private boolean isTouch = false;

    String totalTime;//总时长

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surface);
        seekLayout = findViewById(R.id.seekLayout);
        seekBar = findViewById(R.id.seekBar);
        control = findViewById(R.id.control);
        stop = findViewById(R.id.stop);
        localStart = findViewById(R.id.localStart);
        netStart = findViewById(R.id.netStart);
        time = findViewById(R.id.time);
        name = findViewById(R.id.name);
        seekBar.setOnSeekBarChangeListener(this);
        control.setOnClickListener(this);
        stop.setOnClickListener(this);
        localStart.setOnClickListener(this);
        netStart.setOnClickListener(this);

//        memoryPrint();
        permission();
    }

    private void init(){
        wonderfulPlayer = new WonderfulPlayer("");
        wonderfulPlayer.setSurfaceView(surfaceView);
        wonderfulPlayer.setJniCallback(new WonderfulPlayer.JniCallback() {
            @Override
            public void prepareSuccess() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        wonderfulPlayer.getDurationForNative();//获取总时长
                        initPlayTime();
                        wonderfulPlayer.setPlayState(PlayState.PLAYING);
                        control.setText("pause");
                        wonderfulPlayer.start();
                        Toast.makeText(MainActivity.this,"准备工作已完成，可以播放了！",Toast.LENGTH_SHORT).show();
                    }
                });
            }

            @Override
            public void error(@ErrorType int code) {
                switch (code){
                    case ErrorType.OPEN_FAIL:
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(MainActivity.this,"打开文件失败！path: " + wonderfulPlayer.getDataSource(),Toast.LENGTH_SHORT).show();
                            }
                        });
                        break;
                    case 0:
                        break;
                    default:break;
                }
            }

            @Override
            public void progressUpdate(final double progress) {
                if (wonderfulPlayer.getDuration() == 0)return;
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
//                        //当没有正在手动改变进度条时才自动更新
                        if (!isTouch){
                            int currentTime = (int)(wonderfulPlayer.getDuration() * progress);
                            String currentMinutes = getMinutes(currentTime);
                            String currentSeconds = getSeconds(currentTime);
                            String times = currentMinutes + ":" + currentSeconds + "/" + totalTime;
                            time.setText(times);
                            seekBar.setProgress((int)(progress * 100));
                        }
                    }
                });
            }
        });
    }

    //初始化播放时间
    private void initPlayTime(){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                //duration等于零说明是直播类型，没有进度条
                if (wonderfulPlayer.getDuration() != 0){
                    String minutes = getMinutes(wonderfulPlayer.getDuration());
                    String seconds = getSeconds(wonderfulPlayer.getDuration());
                    totalTime = minutes + ":" + seconds;
                    time.setText("00:00/" + totalTime);
                    seekLayout.setVisibility(View.VISIBLE);
                }
            }
        });
    }

    private String getMinutes(int duration){
        int minutes = duration / 60;
        if (minutes <= 9) {
            return "0" + minutes;
        }
        return "" + minutes;
    }

    private String getSeconds(int duration){
        int seconds = duration % 60;
        if (seconds <= 9) {
            return "0" + seconds;
        }
        return "" + seconds;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,String[] permissions, int[] grantResults){
        switch (requestCode){
            case 1:
                if (grantResults.length>0&&grantResults[0]== PackageManager.PERMISSION_GRANTED){
                    init();
                }else{
                    Toast.makeText(this,"拒绝权限将无法使用此功能",Toast.LENGTH_SHORT).show();
                }
                break;
            default:
        }
    }

    private void permission(){
        if(ContextCompat.checkSelfPermission(this, Manifest.permission.
                WRITE_EXTERNAL_STORAGE)!= PackageManager.PERMISSION_GRANTED){
            ActivityCompat.requestPermissions(this,new String[]
                    {Manifest.permission.WRITE_EXTERNAL_STORAGE},1);
        }else {
            init();
        }
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (fromUser){
            String currentMinutes = getMinutes(wonderfulPlayer.getDuration() * progress / 100);
            String currentSeconds = getSeconds(wonderfulPlayer.getDuration() * progress / 100);
            String times = currentMinutes + ":" + currentSeconds + "/" + totalTime;
            time.setText(times);
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        isTouch = true;
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        isTouch = false;
        double progress = seekBar.getProgress()/100.0;
        wonderfulPlayer.seek(progress);
    }

    @Override
    public void onClick(View v) {
        if (v.getId() == R.id.control){
            if(wonderfulPlayer.getPlayState() != PlayState.PLAYING){
                wonderfulPlayer.playControl(PlayState.PLAYING);
                control.setText("pause");
            }else {
                wonderfulPlayer.playControl(PlayState.PAUSE);
                control.setText("play");
            }
        }
        if (v.getId() == R.id.stop){
            wonderfulPlayer.stop();
        }
        if (v.getId() == R.id.localStart){
            String path = new File(Environment.getExternalStorageDirectory() + "/move/" + name.getText().toString()).getAbsolutePath();
            wonderfulPlayer.setDataSource(path);
            wonderfulPlayer.prepare();
        }
        if (v.getId() == R.id.netStart){
            String path = name.getText().toString();
            wonderfulPlayer.setDataSource(path);
            wonderfulPlayer.prepare();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        wonderfulPlayer.stop();
    }

    private void memoryPrint(){

        new Thread(new Runnable() {
            @Override
            public void run() {
                while(true){
                    try {
                        ActivityManager activityManager=(ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
                        ActivityManager.MemoryInfo memInfo=new ActivityManager.MemoryInfo();
                        activityManager.getMemoryInfo(memInfo);

                        Log.d(TAG, "系统总内存-totalMem: " + memInfo.totalMem/1024 + " kb");
                        Log.d(TAG, "系统可用内存-availMem: " + memInfo.availMem/1024 + " kb");
                        Log.d(TAG, "低内存运行阈值-threshold: " + memInfo.threshold/1024 + " kb");
                        Log.d(TAG, "警告!低内存运行-lowMemory: " + memInfo.lowMemory);
                        Log.d(TAG, "===========================java=========================");

                        Log.d(TAG, "native堆大小: " + Debug.getNativeHeapSize()/1024 + " kb");
                        Log.e(TAG, "native堆已占用大小: " + Debug.getNativeHeapAllocatedSize()/1024 + " kb");
                        Log.d(TAG, "native堆可用大小: "+ Debug.getNativeHeapFreeSize()/1024 + " kb");//low memory threshold
                        Log.d(TAG, "===========================native=========================");

                        Thread.sleep(2000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }).start();
    }
}
