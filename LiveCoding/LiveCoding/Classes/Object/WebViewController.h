//
//  WebVIewController.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 14..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef void(^didFinishLoad)();

@interface WebViewController : NSObject <UIWebViewDelegate>

@property (strong, nonatomic) UIWebView *webView;
@property (assign, nonatomic) BOOL webViewLock;
@property (copy, nonatomic) didFinishLoad didFinishLoad;
@property (assign, nonatomic) BOOL isLogedIn;
@property (assign, nonatomic) BOOL logingIn;
@property (assign, nonatomic) BOOL logingOut;

+(void)checkLoginStatus:(NSData *)data;
+(WebViewController *)sharedInstance;
+(void)login;
+(void)logout;
-(void)loadHtml:(NSURL *)url completed:(didFinishLoad)finish;

@end
