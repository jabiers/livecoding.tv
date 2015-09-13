//
//  ScheduleTableViewCell.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 13..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "ScheduleTableViewCell.h"

@implementation ScheduleTableViewCell

- (void)awakeFromNib {
    // Initialization code
}

-(void)setScheduleEntity:(ScheduleEntity *)scheduleEntity {
    
    [self.timeLabel setText:scheduleEntity.time];
    [self.titleLabel setText:scheduleEntity.title];
    [self.authorLabel setText:scheduleEntity.author];
    [self.languageLabel setText:scheduleEntity.language];
    [self.liveLabel setHidden:!scheduleEntity.isLive];
    
    if (scheduleEntity.isLive) {
        [self.backGroundItemView setBackgroundColor:[UIColor redColor]];
    } else {
        [self.backGroundItemView setBackgroundColor:[UIColor colorWithRed:0 green:0 blue:0 alpha:0.1f]];
    }
    
    self.backGroundItemView.layer.cornerRadius = 5.0f;
    self.backGroundItemView.layer.borderColor = [[UIColor lightGrayColor] CGColor];
    self.backGroundItemView.layer.borderWidth = 1.0f;
    
}


@end
