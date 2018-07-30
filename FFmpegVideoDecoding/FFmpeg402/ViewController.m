//
//  ViewController.m
//  FFmpeg01
//
//  Created by bigfish on 2018/7/25.
//  Copyright © 2018年 bigfish. All rights reserved.
//

#import "ViewController.h"
#import "FFmpegTest.h"
@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    [FFmpegTest ffmpegTestConfig];
    NSString *path =  [[NSBundle mainBundle] pathForResource:@"cat" ofType:@".mp4"];
    [FFmpegTest ffmpegOpenFile:path];
}





@end
