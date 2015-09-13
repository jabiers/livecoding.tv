//
//  ScheduleTableViewCell.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 13..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ScheDuleEntity.h"

@interface ScheduleTableViewCell : UITableViewCell

@property (strong, nonatomic)  ScheduleEntity *scheduleEntity;
@property (weak, nonatomic) IBOutlet UILabel *timeLabel;
@property (weak, nonatomic) IBOutlet UILabel *titleLabel;
@property (weak, nonatomic) IBOutlet UILabel *authorLabel;
@property (weak, nonatomic) IBOutlet UILabel *languageLabel;
@property (weak, nonatomic) IBOutlet UILabel *liveLabel;
@property (weak, nonatomic) IBOutlet UIView *backGroundItemView;
@end
