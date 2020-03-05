package com.example.wywonderfulplayer;

import androidx.annotation.IntDef;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * 限定范围，这是一个元注解
 */
@IntDef({
        PlayState.STOP,
        PlayState.PLAYING,
        PlayState.PAUSE
})
@Target({
        ElementType.PARAMETER,
        ElementType.FIELD,
        ElementType.METHOD
})
@Retention(RetentionPolicy.SOURCE)
//注意需要和native层对应
public @interface PlayState {
    int STOP = 0;   //停止
    int PLAYING = 1;//正在播放
    int PAUSE = 2;  //暂停
}
