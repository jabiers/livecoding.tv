//
//  Configs.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 6..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <Foundation/Foundation.h>

#define HOST_NAME @"https://www.livecoding.tv"
#define APP_DELEGATE ((AppDelegate *)[[UIApplication sharedApplication] delegate])

#define LEFT_PANEL_LIST @[@"LiveStream", @"Videos", @"PlayList", @"Schedule", @"login"]
#define DONATION_LIST @[@"$ 5.00", @"$10.00", @"$20.00",@"$30.00",@"$40.00",@"$50.00",@"$100.00",@"$150.00",@"$200.00",@"$250.00",@"$300.00",@"$500.00",@"$1000.00"]

@interface Configs : NSObject

@end
