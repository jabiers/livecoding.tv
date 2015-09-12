//
//  StreamingEntity.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 5..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <Foundation/Foundation.h>


typedef enum {
    StreamTypeLive = 0,
    StreamTypeVideo,
    StreamTypePlayList
}StreamType;

@interface StreamingEntity : NSObject

#pragma mark -
#pragma mark - Properties

@property (assign, nonatomic) StreamType type;
@property (strong, nonatomic) NSString *streamingUrl;
@property (strong, nonatomic) NSString *thumbUrl;
@property (strong, nonatomic) NSString *title;
@property (strong, nonatomic) NSString *contry;
@property (strong, nonatomic) NSString *author;
@property (strong, nonatomic) NSString *authorAvatar;
@property (strong, nonatomic) NSString *expert;
@property (strong, nonatomic) NSString *numberOfViews;
@property (strong, nonatomic) NSString *numberOfVideos;
@property (strong, nonatomic) NSString *language;
//@property (strong, nonatomic) NSString *

@end
