//
//  StreamListViewController.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 5..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "ESViewController.h"
#import "StreamingEntity.h"
#import "StreamPlayerViewController.h"

@interface StreamListViewController : ESViewController <UITableViewDataSource, UITableViewDelegate>

#pragma mark -
#pragma mark - Properties
@property (weak, nonatomic) IBOutlet UITableView *tableView;
@property (strong, nonatomic) NSMutableArray *streamItems;

@end
