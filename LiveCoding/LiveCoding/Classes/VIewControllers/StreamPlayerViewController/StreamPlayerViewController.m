//
//  StreamPlayerViewController.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 6..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "StreamPlayerViewController.h"
//#import "VideoViewController.h"
#import <gst/player/gstplayer.h>

@implementation StreamPlayerViewController {
    GstPlayer *player;
    int media_width;                /* Width of the clip */
    int media_height;               /* height ofthe clip */
    Boolean dragging_slider;        /* Whether the time slider is being dragged or not */
    Boolean is_local_media;         /* Whether this clip is stored locally or is being streamed */
    Boolean is_playing_desired;     /* Whether the user asked to go to PLAYING */
}

- (void)gstPlayerInit {
    media_width = 320;
    media_height = 240;
    GstPlayerVideoRenderer *renderer = gst_player_video_overlay_video_renderer_new ((__bridge gpointer)([self video_view]));
    player = gst_player_new_full (renderer, NULL);
    g_object_set (player, "uri", [[self uri] UTF8String], NULL);
    
    gst_debug_set_threshold_for_name("gst-player", GST_LEVEL_TRACE);
    
    g_signal_connect (player, "position-updated", G_CALLBACK (position_updated), (__bridge gpointer) self);
    g_signal_connect (player, "duration-changed", G_CALLBACK (duration_changed), (__bridge gpointer) self);
    g_signal_connect (player, "video-dimensions-changed", G_CALLBACK (video_dimensions_changed), (__bridge gpointer) self);
    
    is_local_media = [[self uri] hasPrefix:@"file://"];
    is_playing_desired = NO;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self setTitle:[[self entity] title]];
    [[self thumbImageView] setImageUrl:[[self entity] thumbUrl]];
    
    AFHTTPRequestOperationManager* manager = [AFHTTPRequestOperationManager manager];
    manager.responseSerializer = [AFHTTPResponseSerializer serializer];
    
    [manager GET:[[self entity] streamingUrl] parameters:nil success:^(AFHTTPRequestOperation *operation, id responseObject) {
        NSString *sourceString = [[NSString alloc] initWithData:responseObject encoding:NSUTF8StringEncoding];
        NSString *rtmp;
        @try {
            NSRange start = [sourceString rangeOfString: @"rtmp"];
            NSString *startString = [sourceString substringFromIndex:start.location];
            NSRange end = [startString rangeOfString: @"\","];
            rtmp = [startString substringToIndex:end.location];
            
        } @catch (NSException * e) {
            UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:@"error" message:@"need to login" delegate:nil cancelButtonTitle:nil otherButtonTitles:@"Ok", nil];
            [alertView show];
        }
        
        if (rtmp) {
            NSLog(@"rtmp : %@", rtmp);
            [self setUri:rtmp];
            [self gstPlayerInit];
        }

        [self onPlayButtonClicked:nil];

    } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
        NSLog(@"Error: %@", error);
    }];
    
    NSString *chatUrlString = [NSString stringWithFormat:@"%@/chat/%@", HOST_NAME, [[self entity] author]];
    
    NSURL *chatUrl = [NSURL URLWithString:chatUrlString];
    NSURLRequest *request = [NSURLRequest requestWithURL:chatUrl];
    [self.webView loadRequest:request];
    
    
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTapGestureClicked:)];
    [self.video_view addGestureRecognizer:tapGesture];

}

-(void)onTapGestureClicked:(id)sender {
    if ([self.playPauseButton isHidden]) {
        [self.playPauseButton setHidden:NO];
        [self.playPauseButton setAlpha:0.0f];
        [UIView animateWithDuration:0.5f animations:^{
            [self.playPauseButton setAlpha:1.0f];
        } completion:^(BOOL finished) {

        }];
    }
}

- (void)viewDidDisappear:(BOOL)animated
{
    if (player)
    {
        gst_object_unref (player);
    }
    [UIApplication sharedApplication].idleTimerDisabled = NO;
}


-(IBAction)onButtonClicked:(id)sender {
    if (![self.playPauseButton isSelected]) {
        [self onPlayButtonClicked:nil];
    } else {
        [self onPauseButtonClicked:nil];
    }
}

-(IBAction)onFollowButtonClicked:(id)sender {
    
}

-(IBAction)onDonateButtonClicked:(id)sender {
}

-(IBAction)onHireButtonClicked:(id)sender {
    
}

-(IBAction)onLiveButtonClicked:(id)sender {
    
}

-(void)onPlayButtonClicked:(id)sender {
    gst_player_play (player);
    is_playing_desired = YES;
    [self.playPauseButton setSelected:YES];
    [UIApplication sharedApplication].idleTimerDisabled = YES;
    
    [self.playPauseButton setAlpha:1.0f];
    [UIView animateKeyframesWithDuration:0.5f delay:2.0f options:UIViewKeyframeAnimationOptionAllowUserInteraction animations:^{
        [self.playPauseButton setAlpha:0.0f];
    } completion:^(BOOL finished) {
        [self.playPauseButton setHidden:YES];
    }];
}

-(void)onPauseButtonClicked:(id)sender {
    gst_player_pause(player);
    is_playing_desired = NO;
    [self.playPauseButton setSelected:NO];
    [UIApplication sharedApplication].idleTimerDisabled = NO;
    
    [self.playPauseButton setHidden:NO];
    [self.playPauseButton setAlpha:0.0f];
    [UIView animateWithDuration:0.5f animations:^{
        [self.playPauseButton setAlpha:1.0f];
    } completion:^(BOOL finished) {
    }];
}

#pragma mark -
#pragma mark - Private Methods

-(IBAction)hiddemButtonClicked:(id)sender {
    if (self.webViewHeightConstraint.constant > 0) {
        self.webViewHeightConstraint.constant = 0;
    } else {
        self.webViewHeightConstraint.constant = 200;
    }

}

#pragma mark -
#pragma mark - C Methods

static void video_dimensions_changed (GstPlayer * unused, gint width, gint height, StreamPlayerViewController * self)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (width > 0 && height > 0) {
            [self videoDimensionsChanged:width height:height];
        }
    });
}

-(void) videoDimensionsChanged:(NSInteger)width height:(NSInteger)height
{
    media_width = width;
    media_height = height;
    [self viewDidLayoutSubviews];
    [[self video_view] setNeedsLayout];
    [[self video_view] layoutIfNeeded];
}

static void position_updated (GstPlayer * unused, GstClockTime position, StreamPlayerViewController *self)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self positionUpdated:(int) (position / 1000000)];
    });
}

-(void) positionUpdated:(NSInteger)position
{
    /* Ignore messages from the pipeline if the time sliders is being dragged */
    if (dragging_slider) return;
    
//    time_slider.value = position;
//    [self updateTimeWidget];
}

static void duration_changed (GstPlayer * unused, GstClockTime duration, StreamPlayerViewController *self)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self durationChanged:(int) (duration / 1000000)];
    });
}

-(void) durationChanged:(NSInteger)duration
{
//    time_slider.maximumValue = duration;
//    [self updateTimeWidget];
}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
//    VideoViewController *destViewController = segue.destinationViewController;
//    destViewController.title =@"우왕~!";
//    destViewController.uri = @"rtmp://eumedia1.livecoding.tv:1935/livecodingtv/paulkim?t=9FE3CA31501C4ABCB84AEF14D1943F4E";
}


-(IBAction)onBackButtonClicked:(id)sender {
    [[self navigationController] popViewControllerAnimated:YES];
}

@end
