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

typedef enum {
    StreamListViewModeLive,
    StreamListViewModeTopVideos,
    StreamListViewModePlayList
} StreamListViewMode;

@interface StreamListViewController : ESViewController <
UICollectionViewDataSource,
UICollectionViewDelegate>

#pragma mark -
#pragma mark - Properties

@property (assign, nonatomic) StreamListViewMode viewMode;
@property (weak, nonatomic) IBOutlet UICollectionView *collectionView;
@property (strong, nonatomic) NSMutableArray *streamLiveItems;
@property (strong, nonatomic) NSMutableArray *streamVideoItems;

-(void)reload;
@end
