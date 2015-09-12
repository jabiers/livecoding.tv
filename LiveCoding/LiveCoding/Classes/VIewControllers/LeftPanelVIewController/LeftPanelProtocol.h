//
//  LeftPanelProtocol.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 12..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//


@protocol LeftPanelDelegate <NSObject>

-(void)leftPanel:(LeftPanelViewController *)leftPanel didSearch:(NSString *)search;
-(void)leftPanel:(LeftPanelViewController *)leftPanel didSelectedIndex:(NSIndexPath *)indexPath;

@end
