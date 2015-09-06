//
//  MenuViewController.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 6..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "MenuViewController.h"
#import "AppDelegate.h"

#define MENU_LIST @[@"Blog", @"Roadmap", @"Pastebin", @"Visual Answerbase", @"Statistics", @"Press", @"Goodies", @"Built on Livecoding.tv", @"StreamerProgram", @"Streamer Guide", @"Support", @"Contact", @"About Us"]
#define MENU_URL_LIST @[@"http://blog.livecoding.tv/",\
@"http://roadmap.livecoding.tv/",\
@"https://www.livecoding.tv/pastebin/",\
@"https://www.livecoding.tv/answerbase/",\
@"https://www.livecoding.tv/statistics/streamers/",\
@"https://www.livecoding.tv/press/",\
@"https://www.livecoding.tv/goodies/",\
@"https://www.livecoding.tv/builtonlivecodingtv/",\
@"https://www.livecoding.tv/streamerprogram/",\
@"https://www.livecoding.tv/streamingguide/",\
@"https://www.livecoding.tv/support/",\
@"https://www.livecoding.tv/contact/",\
@"https://www.livecoding.tv/about/"]

@implementation MenuViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    [self setTitle:@"메뉴"];
    // Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark -
#pragma mark - UITableView Delegate

-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    NSString *urlString = [MENU_URL_LIST objectAtIndex:[indexPath row]];
    NSURL *url = [NSURL URLWithString:urlString];
    [[UIApplication sharedApplication] openURL:url];
}

#pragma mark -
#pragma mark - UITableView DataSource

-(NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

-(NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [MENU_LIST count];
}

#define CELL @"CELL"
-(UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CELL];
    
    if (!cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL];
    }
    
    [[cell textLabel] setText:[MENU_LIST objectAtIndex:[indexPath row]]];
    
    return cell;
}

@end
