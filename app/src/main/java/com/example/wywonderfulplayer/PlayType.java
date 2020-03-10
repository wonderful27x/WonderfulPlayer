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
        PlayType.LOCAL,
        PlayType.RTMP,
        PlayType.CLICK
})
@Target({
        ElementType.PARAMETER,
        ElementType.FIELD,
        ElementType.METHOD
})
@Retention(RetentionPolicy.SOURCE)
//播放类型
public @interface PlayType {
    int LOCAL = 0;      //本地流
    int RTMP = 1;       //RTMP流
    int CLICK = 2;      //点播
}
