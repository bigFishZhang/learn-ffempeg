//
//  FFmpegTest.h
//  FFmpeg01
//
//  Created by bigfish on 2018/7/25.
//  Copyright © 2018年 bigfish. All rights reserved.
//

#import <Foundation/Foundation.h>





//#import <ios>
@interface FFmpegTest : NSObject
//获取配置
+ (void)ffmpegTestConfig;
//打开一个视频文件
+ (void)ffmpegOpenFile:(NSString *)filePath;
@end
