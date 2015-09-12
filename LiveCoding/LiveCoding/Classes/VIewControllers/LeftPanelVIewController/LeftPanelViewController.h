//
//  LeftPanelViewController.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 12..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "ESViewController.h"
#import "LeftPanelProtocol.h"

@interface LeftPanelViewController : ESViewController <
UISearchBarDelegate,
UITableViewDataSource,
UITableViewDelegate>

@property (weak, nonatomic) IBOutlet UISearchBar *searchBar;
@property (weak, nonatomic) id <LeftPanelDelegate> delegate;

+(LeftPanelViewController *)viewControllerWithDelegate:(id<LeftPanelDelegate>)delegate;

@end

