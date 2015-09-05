//
//  StreamListViewController.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 5..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "StreamingEntity.h"

@interface StreamListViewController : UIViewController <UITableViewDataSource, UITableViewDelegate>

#pragma mark -
#pragma mark - Properties
@property (weak, nonatomic) IBOutlet UITableView *tableView;
@property (strong, nonatomic) NSMutableArray *streamItems;
@end
