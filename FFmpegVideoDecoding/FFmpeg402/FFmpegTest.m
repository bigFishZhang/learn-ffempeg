//
//  FFmpegTest.m
//  FFmpeg01
//
//  Created by bigfish on 2018/7/25.
//  Copyright © 2018年 bigfish. All rights reserved.
//

#import "FFmpegTest.h"

@implementation FFmpegTest
//获取配置
+ (void)ffmpegTestConfig{
    const char *config =avcodec_configuration();
    NSLog(@"配置信息 %s",config);
}

//打开一个视频文件
+ (void)ffmpegOpenFile:(NSString *)filePath{
    // 1 注册组件
//    av_register_all();
    
    // 2 初始化网络 如果需要的话
    avformat_network_init();
    
    // 2 打开封装格式文件
    //封装格式上下文
    AVFormatContext *avformat_context = avformat_alloc_context();
    
    //文件路径
    const char *url = [filePath UTF8String];
    
    /*
     • AVFormatContext **ps  传指针的地址
     • const char *url   文件路径（本地的或者网络的http rtsp 地址会被存在AVFormatContext 结构体的 fileName中）
     • AVInputFormat *fmt 指定输入的封装格式 一般情况传NULL即可，自行判断即可
     • AVDictionary **options 一般传NULL
     */
    int avformat_open_input_result = avformat_open_input(&avformat_context, url, NULL, NULL);
    
    if (avformat_open_input_result != 0) {
        NSLog(@"打开文件失败");
        char *error_info = NULL;
        av_strerror(avformat_open_input_result, error_info, 1024);
        
        return;
    }
    
    NSLog(@"打开文件成功");
    //获取流信息 读取部分视频做探测
    avformat_open_input_result = avformat_find_stream_info(avformat_context, 0);
    if (avformat_open_input_result != 0) {
        NSLog(@"avformat_find_stream_info failed!");
    }
    //总时长，流的信息
    NSLog(@"duration =%lld nb_streams = %d",avformat_context->duration,avformat_context->nb_streams);
    //关闭输入的上下文 释放内存
    avformat_close_input(&avformat_context);
    
}
@end
