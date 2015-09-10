//
//  StreamCollectionViewCell.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 10..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "StreamCollectionViewCell.h"

@implementation StreamCollectionViewCell

-(void)awakeFromNib {
    
}

-(void)setSelected:(BOOL)selected {
    
    [super setSelected:selected];
    
}
-(void)setStreamingEntity:(StreamingEntity *)entity {
    if (entity) {
        [self.authorNameLabel setText:entity.author];
        [self.titleLabel setText:entity.title];
        [self.expertLabel setText:entity.expert];
        [self.viewerLabel setText:entity.numberOfViews];
        [self.mainImageView setImageUrl:entity.thumbUrl];
        [self.contryImageView setImageUrl:entity.contry];
    }
}

@end
