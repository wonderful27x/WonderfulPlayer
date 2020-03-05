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
    ErrorType.OPEN_FAIL
})
@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.SOURCE)
public @interface ErrorType {
    int OPEN_FAIL = 1001;      //打开文件失败
}
