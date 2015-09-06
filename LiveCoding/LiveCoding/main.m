//
//  main.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 5..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#include "gst_ios_init.h"

int main(int argc, char * argv[]) {
    @autoreleasepool {
        gst_ios_init();
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
