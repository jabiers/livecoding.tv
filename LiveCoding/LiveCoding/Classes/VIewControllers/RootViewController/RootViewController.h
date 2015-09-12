//
//  RootViewController.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 12..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "ESViewController.h"
#import "LeftPanelProtocol.h"

@interface RootViewController : ESViewController <LeftPanelDelegate>

@property (weak, nonatomic) IBOutlet UIView *viewControllerContainer;
@property (weak, nonatomic) UINavigationController *navigationController;

@end