//
//  FFmpegTest.m
//  FFmpeg01
//
//  Created by bigfish on 2018/7/25.
//  Copyright © 2018年 bigfish. All rights reserved.
//

#import "FFmpegTest.h"

static double r2d(AVRational r){
    return r.num == 0 || r.den ==0 ? 0: (double)r.num / (double)r.den;
}


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
    
    // 初始化 解码器
//    avcodec_register_all();
    
    // 3 打开封装格式文件
    //封装格式上下文
    AVFormatContext *ic = avformat_alloc_context();
    
    //文件路径
    const char *url = [filePath UTF8String];
    
    /*
     • AVFormatContext **ps  传指针的地址
     • const char *url   文件路径（本地的或者网络的http rtsp 地址会被存在AVFormatContext 结构体的 fileName中）
     • AVInputFormat *fmt 指定输入的封装格式 一般情况传NULL即可，自行判断即可
     • AVDictionary **options 一般传NULL
     */
    int re = avformat_open_input(&ic, url, NULL, NULL);
    
    if (re != 0) {
        NSLog(@"打开文件失败");
        char *error_info = NULL;
        av_strerror(re, error_info, 1024);
        
        return;
    }
    
    NSLog(@"打开文件成功");
    //获取流信息 读取部分视频做探测
    re = avformat_find_stream_info(ic, 0);
    if (re != 0) {
        NSLog(@"avformat_find_stream_info failed!");
    }
    //总时长，流的信息
    NSLog(@"duration =%lld nb_streams = %d",ic->duration,ic->nb_streams);
    //打印相关信息
    int fps = 0;
    int videoStream = 0;
    int audioStream = 1;
    //(1)遍历
    for (int i = 0; i < ic->nb_streams; i++) {
        AVStream *as = ic->streams[i];
        if (as->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            NSLog(@"VIDEO");
            videoStream = i;
            fps = r2d(as->avg_frame_rate);
            NSLog(@"fps = %d width = %d  height = %d codeid = %d pixformat = %d",
                  fps,
                  as->codecpar->width,
                  as->codecpar->height,
                  as->codecpar->codec_id,
                  as->codecpar->format);
        }
        if (as->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            NSLog(@"AUDIO");
            audioStream = i;
            NSLog(@"sample_rate = %d channels = %d sample_format = %d",
                  as->codecpar->sample_rate,
                  as->codecpar->channels,
                  as->codecpar->format);
        }
    }
    
    
    // ====================================
    /**
     获取音视频流的索引

     @param ic#> 上下文 description#>
     @param type#> 音频/视频流信息类型 description#>
     @param wanted_stream_nb#> 指定取的流的信息（传 -1） description#>
     @param related_stream#> 想关流信息（-1） description#>
     @param decoder_ret#> 解码器 （NULL） description#>
     @param flags#> 暂时无用 description#>
     @return <#return value description#>
     */
    audioStream = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    NSLog(@" av_find_best_stream : %d ",audioStream);
    
    //软解码器
    AVCodec *codec = avcodec_find_decoder(ic->streams[videoStream]->codecpar->codec_id);
    //硬解码
//    codec = avcodec_find_decoder_by_name("h264_mediacodec");
    if (!codec) {
        NSLog(@"avcodec_find_decoder_by_name  video failed!");
        return ;
    }
    //解码器初始化
    AVCodecContext *vc = avcodec_alloc_context3(codec);
    vc->thread_count = 1;
    avcodec_parameters_to_context(vc, ic->streams[videoStream]->codecpar);
    //打开解码器
    re = avcodec_open2(vc,0, 0);
    if (re != 0) {
         NSLog(@"avcodec_open2 video failed!");
         return;
    }
    
    ////////////////////////////////////////////////////
    //音频解码器
    AVCodec *acodec = avcodec_find_decoder(ic->streams[audioStream]->codecpar->codec_id);
    //硬解码
    //    codec = avcodec_find_decoder_by_name("h264_mediacodec");
    if (!acodec) {
        NSLog(@"avcodec_find_decoder_by_name audio failed!");
        return ;
    }
    //解码器初始化
    AVCodecContext *ac = avcodec_alloc_context3(acodec);
    ac->thread_count = 1;
    avcodec_parameters_to_context(ac, ic->streams[audioStream]->codecpar);
    //打开解码器
    re = avcodec_open2(ac,0, 0);
    if (re != 0) {
        NSLog(@"avcodec_open2 audio failed!");
        return;
    }
    
    //读取帧数据
    AVPacket *pkt = av_packet_alloc();//创建一个对象空间，初始化
    AVFrame *frame = av_frame_alloc();
    for (; ; ) {//无线循环
        int re = av_read_frame(ic, pkt);
  
        
        if (re != 0) {
            NSLog(@"读取到结尾处！");
            int pos = 3 * r2d(ic->streams[videoStream]->time_base);;
            av_seek_frame(ic, videoStream, pos, AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME);
            continue;
        }
        
        //测试音视频解码
        AVCodecContext *cc = vc;
        if (pkt->stream_index == audioStream) {
            cc = ac;
        }
        
        //发送的线程中解码
        re = avcodec_send_packet(cc, pkt);\
        //解码
        av_packet_unref(pkt);
        
        if (re != 0) {
            NSLog(@"avcodec_send_packet audio failed!");
            continue;
        }
        for (; ; ) {//保证能搜到全部的解码数据
            re = avcodec_receive_frame(cc, frame);
            if (re != 0) {
//                NSLog(@"avcodec_receive_frame audio failed!");
                break;
            }
            NSLog(@"frame->pts %lld",frame->pts);
        }
        //最后一针的时候 avcodec_send_packet(cc, NULL)
        
        
    
        
        
        
//        NSLog(@"stream = %d size = %d pts = %lld flag = %d",pkt->stream_index,
//              pkt->size,pkt->pts,pkt->flags);
        /////操作

        av_packet_unref(pkt);
    }

    //关闭输入的上下文 释放内存
    avformat_close_input(&ic);
    
    
}



@end
