//
//  StreamPlayerViewController.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 6..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "ESViewController.h"
#import "StreamingEntity.h"
#import "ESImageView.h"

@interface StreamPlayerViewController : ESViewController <UIWebViewDelegate>

@property (weak, nonatomic) IBOutlet ESImageView *thumbImageView;
@property (weak, nonatomic) IBOutlet UIView *video_view;
@property (weak, nonatomic) IBOutlet UIView *video_container_view;
@property (weak, nonatomic) IBOutlet UIWebView *webView;
@property (weak, nonatomic) IBOutlet UIButton *playPauseButton;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *webViewHeightConstraint;

@property (strong, nonatomic) StreamingEntity *entity;
@property (retain,nonatomic) NSString *uri;

//-(IBAction) play:(id)sender;
//-(IBAction) pause:(id)sender;
//-(IBAction) sliderValueChanged:(id)sender;
//-(IBAction) sliderTouchDown:(id)sender;
//-(IBAction) sliderTouchUp:(id)sender;

@end
