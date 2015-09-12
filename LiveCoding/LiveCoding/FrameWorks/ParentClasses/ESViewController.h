//
//  ESViewController.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 6..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <UIKit/UIKit.h>
typedef void(^ViewControllerRefresh)();

@class LeftPanelViewController;
@interface ESViewController : UIViewController

@property (weak, nonatomic) LeftPanelViewController *leftPanel;
@property (assign, nonatomic) BOOL isShownleftPanel;
@property (strong, nonatomic) UIView *alertBackGroundView;
@property (strong, nonatomic) UIRefreshControl *refreshControl;
@property (assign, nonatomic) ViewControllerRefresh refreshAction;
-(void)attachLeftPanel;
-(void)showLeftPanelIfNeed;


- (void)setRefreshWithScrollView:(UIScrollView *)scrollView
                      withAction:(ViewControllerRefresh)action;

@end
