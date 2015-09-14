//
//  WebVIewController.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 14..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "WebViewController.h"
#import "UIAlertView+Blocks.h"

@implementation WebViewController


+(WebViewController *)sharedInstance {
    static WebViewController *instance;
    
    if (!instance) {
        instance = [[WebViewController alloc] init];
    }
    
    return instance;
}


-(instancetype)init {
    if (self = [super init]) {
        self.webView = [[UIWebView alloc] init];
        self.webView.delegate = self;
        self.logingIn = NO;
    }
    return self;
}

#pragma mark -
#pragma mark - public Methods

+(void)checkLoginStatus:(NSData *)data {
    
    TFHpple *xpath = [[TFHpple alloc] initWithData:data
                                             isXML:NO];
    NSArray *elements = [xpath searchWithXPathQuery:@"//html//body//section//aside//nav/ul//li//ul[@class='main-sub-menu']//li//a"]; // <-- tags
    
    BOOL hasLogout = NO;
    for (TFHppleElement *element in elements) {
        if (element.content) {
            if ([element.content isEqualToString:@"Logout"]) {
                hasLogout = YES;
            }
        }
    }
    [WebViewController sharedInstance].isLogedIn = hasLogout;
}

+(void)login {
    if (![WebViewController sharedInstance].logingIn) {
        [WebViewController sharedInstance].logingIn = YES;
        NSURL *url = [NSURL URLWithString:HOST_NAME@"/accounts/login/"];
        NSURLRequest *request = [NSURLRequest requestWithURL:url];
        [[[WebViewController sharedInstance] webView] loadRequest:request];
        [[[WebViewController sharedInstance] webView] setFrame:APP_DELEGATE.window.bounds];
        [APP_DELEGATE.window addSubview:[[WebViewController sharedInstance] webView]];
    }
}

+(void)logout {
    if (![WebViewController sharedInstance].logingOut) {
        [WebViewController sharedInstance].logingOut = YES;
        NSURL *url = [NSURL URLWithString:HOST_NAME@"/accounts/logout/"];
        NSURLRequest *request = [NSURLRequest requestWithURL:url];
        [[[WebViewController sharedInstance] webView] loadRequest:request];
    }
}

-(void)loadHtml:(NSURL *)url completed:(didFinishLoad)finish {
    
    if (!self.webViewLock) {
        self.didFinishLoad = finish;
        self.webViewLock = YES;
        NSURLRequest *request = [NSURLRequest requestWithURL:url];
        [self.webView loadRequest:request];
    }
}

#pragma mark -
#pragma mark - Private Methods

#pragma mark -
#pragma mark - UIWebView Delegate

-(BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType {
    return YES;
}

-(void)webViewDidStartLoad:(UIWebView *)webView {
    
}

-(void)webViewDidFinishLoad:(UIWebView *)webView {
    
    if ([webView.request.URL.absoluteString hasPrefix:@"https://www.livecoding.tv/livestreams/"]) {
        NSString *yourHTMLSourceCodeString = [webView stringByEvaluatingJavaScriptFromString:@"document.body.innerHTML"];

        [WebViewController checkLoginStatus:[yourHTMLSourceCodeString dataUsingEncoding:NSUTF8StringEncoding]];
    }
    
    if (self.logingOut) {
        [UIAlertView showWithTitle:@"" message:@"Logout" style:UIAlertViewStyleDefault cancelButtonTitle:nil otherButtonTitles:@[@"확인"] tapBlock:^(UIAlertView *alertView, NSInteger buttonIndex) {
            
        }];
        
        self.logingOut = NO;
        self.isLogedIn = NO;
    }
    
    if (self.didFinishLoad) {
        self.didFinishLoad();
    }
    
    if (self.logingIn && self.isLogedIn && [webView superview]) {
        self.logingIn = NO;
        [webView removeFromSuperview];
    }
    
    self.webViewLock = NO;
}

-(void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error {
    self.webViewLock = NO;
    NSLog(@"fail error : %@", error);
}
@end
