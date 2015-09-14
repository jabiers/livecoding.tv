//
//  ESViewController.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 6..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "ESViewController.h"
#import "LeftPanelViewController.h"

@implementation ESViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

-(void)viewWillAppear:(BOOL)animated {
    
    [super viewWillAppear:animated];
    if (![self isKindOfClass:[RootViewController class]]
        && ![self isKindOfClass:[LeftPanelViewController class]]) {
        APP_DELEGATE.currentViewController = self;
    }
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void)attachLeftPanel {
    if (!self.leftPanel) {
        [self makeAlertBackGroundView];

        self.leftPanel = [LeftPanelViewController viewControllerWithDelegate:self];

        [self addChildViewController:self.leftPanel];
        [self.view addSubview:self.leftPanel.view];
        
        CGRect r = self.view.bounds;
        r.origin.x -= r.size.width;
        self.leftPanel.view.frame = r;
        self.isShownleftPanel = NO;
    }
}

-(void)makeAlertBackGroundView {
    self.alertBackGroundView = [[UIView alloc] initWithFrame:self.view.bounds];
    [self.alertBackGroundView setBackgroundColor:[UIColor colorWithWhite:0.0f alpha:0.3f]];
    [self.alertBackGroundView setUserInteractionEnabled:YES];
    [self.view addSubview:self.alertBackGroundView];
    
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onAlertBackGroundViewTouched:)];
    [self.alertBackGroundView addGestureRecognizer:tapGesture];
    [self.view bringSubviewToFront:self.alertBackGroundView];
    [self.alertBackGroundView setHidden:YES];

}

-(void)onAlertBackGroundViewTouched:(id)sender {
    [self showLeftPanelIfNeed];
}

-(void)showLeftPanelIfNeed {
    
    if (self.leftPanel) {
        [self.leftPanel reload];
        
        [self.view bringSubviewToFront:self.alertBackGroundView];
        [self.view bringSubviewToFront:self.leftPanel.view];

        if (self.isShownleftPanel) {
            [UIView animateWithDuration:0.3f animations:^{
                CGRect r = self.view.bounds;
                r.origin.x -= r.size.width;
                self.leftPanel.view.frame = r;
                [self.alertBackGroundView setAlpha:0.0f];
            } completion:^(BOOL finished) {
                self.isShownleftPanel = NO;
                [self.alertBackGroundView setHidden:YES];
            }];
            
        } else {
            [self.alertBackGroundView setHidden:NO];
            [self.alertBackGroundView setAlpha:0.0f];
            [UIView animateWithDuration:0.3f animations:^{
                CGRect r = self.view.bounds;
                r.origin.x -= 0;
                self.leftPanel.view.frame = r;
                [self.alertBackGroundView setAlpha:1.0f];
            } completion:^(BOOL finished) {
                self.isShownleftPanel = YES;

            }];
        }
    }
}

- (void)setRefreshWithScrollView:(UIScrollView *)scrollView
                      withAction:(ViewControllerRefresh)action {
    
    if (self.refreshControl) {
        [self.refreshControl removeFromSuperview];
    }
    
    self.refreshControl = [[UIRefreshControl alloc] init];
    [self.refreshControl addTarget:self
                            action:@selector(onRefreshTouched)
                  forControlEvents:UIControlEventValueChanged];
    self.refreshAction = action;
    
    [scrollView addSubview:self.refreshControl];
    
}

-(void)onRefreshTouched {
    if (self.refreshAction) {
        self.refreshAction();
    }
}

-(void)endRefreshControl {
    [self.refreshControl endRefreshing];
}

-(void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    if (self.leftPanel) {
        CGRect r = self.view.bounds;

        NSLog(@"r : %@", NSStringFromCGRect(r));
        if (toInterfaceOrientation == UIInterfaceOrientationLandscapeLeft
            || toInterfaceOrientation == UIInterfaceOrientationLandscapeRight) {
            r.origin.x -= r.size.height;
        }
        if ([self.leftPanel.view superview]) {
            r.origin.x -= r.size.width;
        }

        self.leftPanel.view.frame = r;
        self.isShownleftPanel = NO;

        r.origin.x = 0;
        
        CGFloat tmp = r.size.height;
        
        r.size.height = 1000;
        r.size.width = 1000;
        
        [self.alertBackGroundView setFrame:r];
        [self.alertBackGroundView setHidden:YES];
    }
    
}

@end
