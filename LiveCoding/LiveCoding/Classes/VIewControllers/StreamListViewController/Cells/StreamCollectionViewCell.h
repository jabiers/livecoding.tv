//
//  StreamCollectionViewCell.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 10..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "StreamingEntity.h"
#import "ESImageView.h"

@interface StreamCollectionViewCell : UICollectionViewCell

@property (weak, nonatomic) IBOutlet UILabel *authorNameLabel;
@property (weak, nonatomic) IBOutlet UILabel *titleLabel;
@property (weak, nonatomic) IBOutlet UILabel *expertLabel;
@property (weak, nonatomic) IBOutlet UILabel *viewerLabel;
@property (weak, nonatomic) IBOutlet ESImageView *mainImageView;
@property (weak, nonatomic) IBOutlet ESImageView *contryImageView;
@property (strong, nonatomic) StreamingEntity *streamingEntity;

@end
