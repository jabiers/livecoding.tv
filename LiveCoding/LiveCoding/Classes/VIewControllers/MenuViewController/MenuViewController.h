//
//  MenuViewController.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 6..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "ESViewController.h"

@interface MenuViewController : ESViewController <UITableViewDataSource, UITableViewDelegate>

@property (weak, nonatomic) IBOutlet UITableView *tableView;

@end
