//
//  StreamEntityTableViewCell.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 5..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "StreamingEntity.h"

@interface StreamEntityTableViewCell : UITableViewCell

#pragma mark -
#pragma mark - Properties
@property (weak, nonatomic) IBOutlet UILabel *titleLabel;
@property (weak, nonatomic) IBOutlet UILabel *authorLabel;
@property (weak, nonatomic) IBOutlet UIImageView *thumbImageView;
@property (strong, nonatomic) StreamingEntity *streamingEntity;

#pragma mark -
#pragma mark - Public Methods
// +(StreamEntityTableViewCell *)cell; With Xib

@end
