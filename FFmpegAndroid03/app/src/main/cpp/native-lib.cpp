#include <jni.h>
#include <android/log.h>

//当前C++兼容C语言
extern "C"{
//avcodec:编解码(最重要的库)
#include <libavcodec/avcodec.h>
//avformat:封装格式处理
#include "libavformat/avformat.h"
//avutil:工具库(大部分库都需要这个库的支持)
//#include "libavutil/imgutils.h"

JNIEXPORT void JNICALL Java_demo_androidffmpeg01_bigfish_ffmpegandroid03_FFmpegTest_ffmpegTest
        (JNIEnv *, jobject);
JNIEXPORT void JNICALL Java_demo_androidffmpeg01_bigfish_ffmpegandroid03_FFmpegTest_ffmpegDecoder(
        JNIEnv *env, jobject jobj, jstring jinFilePath);
}

JNIEXPORT void JNICALL Java_demo_androidffmpeg01_bigfish_ffmpegandroid03_FFmpegTest_ffmpegTest(
        JNIEnv *env, jobject jobj) {
    //(char *)表示C语言字符串
    const char *configuration = avcodec_configuration();
    __android_log_print(ANDROID_LOG_INFO,"FFmpeg配置信息：","%s",configuration);
}


JNIEXPORT void JNICALL Java_demo_androidffmpeg01_bigfish_ffmpegandroid03_FFmpegTest_ffmpegDecoder(
        JNIEnv *env, jobject jobj, jstring jinFilePath){
    const char* cinputFilePath = env->GetStringUTFChars(jinFilePath,NULL);

    //第一步：注册所有组件
//    av_register_all();

    //第二步：打开视频输入文件
    //参数一：封装格式上下文->AVFormatContext->包含了视频信息(视频格式、大小等等...)
    AVFormatContext* avformat_context = avformat_alloc_context();
    //参数二：打开文件(入口文件)->url
    int avformat_open_result = avformat_open_input(&avformat_context,cinputFilePath,NULL,NULL);
    if (avformat_open_result != 0){
        //获取异常信息
        char* error_info;
        av_strerror(avformat_open_result, error_info, 1024);
        __android_log_print(ANDROID_LOG_INFO,"main","异常信息：%s",error_info);
        return;
    }

    __android_log_print(ANDROID_LOG_INFO,"main","文件打开成功");
}
