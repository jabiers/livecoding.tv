//
//  Schedule.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 13..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface ScheduleEntity : NSObject

@property (strong, nonatomic) NSString *time;
@property (strong, nonatomic) NSString *title;
@property (strong, nonatomic) NSString *author;
@property (strong, nonatomic) NSString *language;
@property (assign, nonatomic) BOOL isLive;

@end