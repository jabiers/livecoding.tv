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

@interface StreamPlayerViewController : ESViewController <
UIWebViewDelegate,
UIPickerViewDataSource,
UIPickerViewDelegate>

@property (weak, nonatomic) IBOutlet ESImageView *thumbImageView;
@property (weak, nonatomic) IBOutlet UIView *video_view;
@property (weak, nonatomic) IBOutlet UIView *video_container_view;
@property (weak, nonatomic) IBOutlet UIWebView *webView;
@property (weak, nonatomic) IBOutlet UIButton *playPauseButton;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *webViewHeightConstraint;

@property (strong, nonatomic) StreamingEntity *entity;
@property (retain,nonatomic) NSString *uri;


@property (weak, nonatomic) IBOutlet UIView *pickerBackgroundView;
@property (weak, nonatomic) IBOutlet UIView *pickerContainer;
@property (weak, nonatomic) IBOutlet UIPickerView *pickerView;
@property (weak, nonatomic) IBOutlet UIButton *closeButton;
@property (weak, nonatomic) IBOutlet UIButton *donateButton;

@property (strong, nonatomic) NSString *csrfmiddlewaretoken;
@property (strong, nonatomic) NSString *donation;

//-(IBAction) play:(id)sender;
//-(IBAction) pause:(id)sender;
//-(IBAction) sliderValueChanged:(id)sender;
//-(IBAction) sliderTouchDown:(id)sender;
//-(IBAction) sliderTouchUp:(id)sender;

@end
