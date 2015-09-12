//
//  StreamCollectionViewCell.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 10..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "StreamCollectionViewCell.h"
#import <QuartzCore/QuartzCore.h>

@implementation StreamCollectionViewCell


-(void)awakeFromNib {
    self.recIconView.layer.cornerRadius = self.recIconView.bounds.size.width / 2;
    
}
-(void)setSelected:(BOOL)selected {
    
    [super setSelected:selected];
}

-(void)setStreamingEntity:(StreamingEntity *)entity {
    if (entity) {
        [self.authorNameLabel setText:entity.author];
        [self.authorAvatarImageView setImageWithURL:[NSURL URLWithString:entity.authorAvatar]];
        [self.titleLabel setText:entity.title];
        [self.expertLabel setText:entity.expert];
        [self.languageLabel setText:entity.language];
        [self.viewerLabel setText:entity.numberOfViews];
        [self.mainImageView setImage:nil];
        [self.mainImageView setImageWithURL:[NSURL URLWithString:entity.thumbUrl] placeholderImage:nil];
        
        [self.contryImageView setImage:nil];
        [self.contryImageView setImageWithURL:[NSURL URLWithString:entity.contry] placeholderImage:nil];
        
        if (entity.type == StreamTypeLive) {
            [self.recIconView setHidden:NO];
        } else {
            [self.recIconView setHidden:YES];
        }
        
        if (entity.type == StreamTypePlayList) {
            [self.expertLabel setText:@""];
            [self.languageLabel setText:@""];
            [self.viewerLabel setText:entity.numberOfVideos];
        }
//        [self.mainImageView setImageUrl:entity.thumbUrl];
//        [self.contryImageView setImageUrl:entity.contry];
    }
}

@end
