
cmake_minimum_required(VERSION 3.4.1)

set(ffmpegDir ${CMAKE_SOURCE_DIR}/ffmpeg)
aux_source_directory(${CMAKE_SOURCE_DIR} DIR_SRCS)

include_directories(${ffmpegDir}/include)

add_library( # Sets the name of the library.
        wonderful
        SHARED
        ${DIR_SRCS})

find_library( # Sets the name of the path variable.
        log-lib
        log)

#静态库配置
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${ffmpegDir}/${CMAKE_ANDROID_ARCH_ABI}")
#动态库配置 有bug
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${ffmpegDir}/../jniLibs/${CMAKE_ANDROID_ARCH_ABI}")

target_link_libraries( # Specifies the target library.
        wonderful
        -Wl,--start-group
        avcodec
        avfilter
        avformat
        avutil
        swresample
        swscale
        rtmp
        -Wl,--end-group
        z
        ${log-lib}
        android
        OpenSLES)