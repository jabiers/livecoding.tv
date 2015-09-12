//
//  StreamCollectionViewCell.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 10..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "StreamingEntity.h"
#import "UIImageView+AFNetworking.h"

@interface StreamCollectionViewCell : UICollectionViewCell

@property (weak, nonatomic) IBOutlet UIImageView *authorAvatarImageView;
@property (weak, nonatomic) IBOutlet UILabel *authorNameLabel;
@property (weak, nonatomic) IBOutlet UILabel *titleLabel;
@property (weak, nonatomic) IBOutlet UILabel *expertLabel;
@property (weak, nonatomic) IBOutlet UILabel *languageLabel;
@property (weak, nonatomic) IBOutlet UILabel *viewerLabel;
@property (weak, nonatomic) IBOutlet UIImageView *mainImageView;
@property (weak, nonatomic) IBOutlet UIImageView *contryImageView;
@property (weak, nonatomic) IBOutlet UIView *recIconView;
@property (strong, nonatomic) StreamingEntity *streamingEntity;

@end
