//
//  StreamEntityTableViewCell.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 5..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "StreamEntityTableViewCell.h"

@implementation StreamEntityTableViewCell

- (void)awakeFromNib {
    // Initialization code
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated {
    [super setSelected:selected animated:animated];

    // Configure the view for the selected state
}

-(void)setStreamingEntity:(StreamingEntity *)entity {
    if (entity) {
        NSLog(@"title :%@", [entity title]);
        [[self titleLabel] setText:[entity title]];
        [[self authorLabel] setText:[entity author]];
        
        [[self thumbImageView] setImageUrl:[entity thumbUrl]];
    }
}

@end
