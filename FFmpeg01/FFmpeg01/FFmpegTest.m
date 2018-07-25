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
    av_register_all();
    
    // 2 打开封装格式文件
    //封装格式上下文
    AVFormatContext *avformat_context = avformat_alloc_context();
    
    //文件路径
    const char *url = [filePath UTF8String];
    
    //指定的封装格式 NULL默认
    
    //默认配置信息 NULL默认
    
    int avformat_open_input_result = avformat_open_input(&avformat_context, url, NULL, NULL);
    if (avformat_open_input_result != 0) {
        NSLog(@"打开文件失败");
        char *error_info = NULL;
        av_strerror(avformat_open_input_result, error_info, 1024);
        
        return;
    }
    
    NSLog(@"打开文件成功");
    
}
@end
