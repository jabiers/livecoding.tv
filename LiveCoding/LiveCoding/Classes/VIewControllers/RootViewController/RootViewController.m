//
//  RootViewController.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 12..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "RootViewController.h"
#import "LeftPanelViewController.h"
#import "StreamListViewController.h"
#import "ScheduleViewController.h"

@implementation RootViewController

- (void)viewDidLoad {
    APP_DELEGATE.rootViewController = self;

    [super viewDidLoad];
    [self setTitle:@"LiveCoding.TV"];
    [self attachLeftPanel];
    
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - 
#pragma mark - IBActions

-(IBAction)onLeftButtonClicked:(id)sender {
    [self showLeftPanelIfNeed];
}

-(IBAction)onRightButtonClicked:(id)sender {
    
}

#pragma mark -
#pragma mark - LeftPanel Delegate
-(void)leftPanel:(LeftPanelViewController *)leftPanel didSearch:(NSString *)search {
    
}

-(void)leftPanel:(LeftPanelViewController *)leftPanel didSelectedIndex:(NSIndexPath *)indexPath {
    NSString *title = [LEFT_PANEL_LIST objectAtIndex:indexPath.row];

    if ([APP_DELEGATE.currentViewController isKindOfClass:[StreamListViewController class]]) {
        NSString *title = [LEFT_PANEL_LIST objectAtIndex:indexPath.row];

        StreamListViewController *vc = (StreamListViewController *)APP_DELEGATE.currentViewController;
        if ([title isEqualToString:@"LiveStream"]) {
            vc.viewMode = StreamListViewModeLive;
            [vc reload];
            [self setTitle:@"Stream Live"];
        } else if ([title isEqualToString:@"Videos"]) {
            vc.viewMode = StreamListViewModeTopVideos;
            [vc reload];

            [self setTitle:@"Top Videos"];
        } else if ([title isEqualToString:@"PlayList"]) {
            vc.viewMode = StreamListViewModePlayList;
            [vc reload];
            
            [self setTitle:@"PlayLists"];

        } else if ([title isEqualToString:@"Schedule"]) {
            [self performSegueWithIdentifier:@"ScheduleViewController" sender:nil];
        }
    }

    if ([title isEqualToString:@"login"]) {
        if ([WebViewController sharedInstance].isLogedIn) {
            [WebViewController logout];
        } else {
            [WebViewController login];

        }
    }
    
    [self showLeftPanelIfNeed];
}


@end
