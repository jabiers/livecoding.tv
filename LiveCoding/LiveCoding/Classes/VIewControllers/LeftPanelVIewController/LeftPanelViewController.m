//
//  LeftPanelViewController.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 12..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "LeftPanelViewController.h"
#import "LeftPanelItemTableViewCell.h"


@implementation LeftPanelViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

+(LeftPanelViewController *)viewControllerWithDelegate:(id<LeftPanelDelegate>)delegate {
    LeftPanelViewController *leftPanel = [[UIStoryboard storyboardWithName:@"Main" bundle:nil] instantiateViewControllerWithIdentifier:@"LeftPanelViewController"];
    leftPanel.delegate = delegate;
    return leftPanel;
}


-(void)reload {
    [self.tableView reloadData];
}
#pragma mark -
#pragma mark - UISearchBar Delegate

-(void)searchBarSearchButtonClicked:(UISearchBar *)searchBar {
    NSLog(@"search click");
    
    AFHTTPRequestOperationManager* manager = [AFHTTPRequestOperationManager manager];
    manager.responseSerializer = [AFHTTPResponseSerializer serializer];
    
    [manager GET:[NSString stringWithFormat:@"https://www.livecoding.tv/videos/?q=%@", searchBar.text] parameters:nil success:^(AFHTTPRequestOperation *operation, id responseObject) {
        
        NSLog(@"string : %@", [[NSString alloc] initWithData:responseObject encoding:NSUTF8StringEncoding]);
        
        if (self.delegate) {
            if ([self.delegate respondsToSelector:@selector(leftPanel:didSearch:)]) {
                [self.delegate leftPanel:self didSearch:searchBar.text];
            }
        }
    } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
        NSLog(@"Error: %@", error);
    }];
}

#pragma mark -
#pragma mark - UITableView Delegate

-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    
    if (self.delegate) {
        if ([self.delegate respondsToSelector:@selector(leftPanel:didSelectedIndex:)]) {
            [self.delegate leftPanel:self didSelectedIndex:indexPath];
        }
    }
    
    
}
#pragma mark -
#pragma mark - UITableView DataSource

-(NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

-(NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [LEFT_PANEL_LIST count];
}

-(UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    LeftPanelItemTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"LeftPanelItemTableViewCell"];
    
    if (!cell) {
        cell = [[LeftPanelItemTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"LeftPanelItemTableViewCell"];
    }
    
    if ([[LEFT_PANEL_LIST lastObject] isEqualToString:[LEFT_PANEL_LIST objectAtIndex:indexPath.row]]) {
        
        [cell.titleLabel setText:[WebViewController sharedInstance].isLogedIn?@"Logout":@"Login"];
    } else {
        [cell.titleLabel setText:[LEFT_PANEL_LIST objectAtIndex:indexPath.row]];
    }
    
    return cell;
    
}

@end
