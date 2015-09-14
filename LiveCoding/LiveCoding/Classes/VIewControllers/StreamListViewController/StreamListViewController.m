//
//  StreamListViewController.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 5..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "StreamListViewController.h"
#import "StreamEntityTableViewCell.h"
#import "StreamCollectionViewCell.h"
#import <QuartzCore/QuartzCore.h>

@implementation StreamListViewController

- (void)getStreamEntityList:(NSData *)sourceData {
    
    TFHpple *xpath = [[TFHpple alloc] initWithData:sourceData
                                             isXML:NO];
    NSArray *elements = [NSArray array];
    
    [WebViewController checkLoginStatus:sourceData];
    
    elements = [xpath searchWithXPathQuery:@"//html//body//div[@class='browse-main-videos--item']"]; // <-- tags
    
    for (TFHppleElement *element in elements) {
        
        StreamingEntity *entity = [[StreamingEntity alloc] init];
        TFHppleElement *img = [element searchWithXPathQuery:@"//img[@class='thumbnail']"][0];
        entity.thumbUrl =  [NSString stringWithFormat:@"%@%@", HOST_NAME,[img attributes][@"src"]];
        
        TFHppleElement *titleElement = [element searchWithXPathQuery:@"//span//a[@class='woopra_live_click']"][0];
        NSString *title = [titleElement attributes][@"title"];
        if (!title) {
            title = [titleElement text];
        }
        
        entity.title = title;
        entity.streamingUrl = [NSString stringWithFormat:@"%@%@",HOST_NAME, [titleElement attributes][@"href"]];
        
        NSArray *contryFlag = [element searchWithXPathQuery:@"//span//img[@class='country-flag']"];
        if ([contryFlag count] > 0) {
            TFHppleElement *obj = [contryFlag objectAtIndex:0];
            entity.contry = [NSString stringWithFormat:@"%@%@",HOST_NAME,[obj attributes][@"src"]];
        }
        
        NSArray *user_avatar = [element searchWithXPathQuery:@"//span//img[@class='user-avatar']"];
        if ([user_avatar count] > 0) {
            TFHppleElement *obj = [user_avatar objectAtIndex:0];
            entity.authorAvatar = [NSString stringWithFormat:@"%@%@",HOST_NAME,[obj attributes][@"src"]];
        }

        TFHppleElement *nameElement = [element searchWithXPathQuery:@"//span[@class='browse-main-videos--username']"][0];
        NSString *author = [nameElement content];
        
        author = [author stringByReplacingOccurrencesOfString:@"\t" withString:@""];
        author = [author stringByReplacingOccurrencesOfString:@"\n" withString:@""];
        author = [[author componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] componentsJoinedByString:@""];
        
        entity.author = author;
        
        NSArray *item = [element searchWithXPathQuery:@"//span[@class='browse-main-videos--info']"];
        
        TFHppleElement *video_info = item[0];
        
        NSArray *expertElement = [video_info searchWithXPathQuery:@"//span[@class='browse-main-videos--info-item']"];
        TFHppleElement *expert = expertElement[0];
        entity.expert = [[[expert content] componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] componentsJoinedByString:@""];
        
        if ([expertElement count] > 1) {
            entity.language = [expertElement[1] content];
        } else {
            entity.numberOfVideos = [entity.expert stringByReplacingOccurrencesOfString:@"\n" withString:@""];
        }
        
        NSArray *viewerArr = [video_info searchWithXPathQuery:@"//span[@class='browse-main-videos--info-item views']"];
        TFHppleElement *viewer;
        if ([viewerArr count] > 0) {
            viewer = viewerArr[0];
        }
        if (viewer) {
            entity.numberOfViews = [[[[viewer content] componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] componentsJoinedByString:@""] stringByReplacingOccurrencesOfString:@"\n" withString:@""];
        }
        //        NSLog(@"%@", expertElement[2]);
        //        entity.numberOfViews = [expertElement[2] content];
        //        dic[@"expert"] = [item[2] content];
        
        NSArray *redDot = [element searchWithXPathQuery:@"//div[@class='playRedDot']"];
        if ([redDot count] > 0) {
            entity.type = StreamTypeLive;
            [self.streamLiveItems addObject:entity];
        } else {
            
            if (self.viewMode == StreamListViewModeTopVideos) {
                entity.type = StreamTypeVideo;
            } else {
                entity.type = StreamTypePlayList;
            }
            
            [self.streamVideoItems addObject:entity];
        }
    }
    
    [self.collectionView reloadData];
}

-(void)initialize {
    self.streamLiveItems = [NSMutableArray array];
    self.streamVideoItems = [NSMutableArray array];
    self.viewMode = StreamListViewModeLive;

}
- (void)viewDidLoad {
    [super viewDidLoad];
    
    [self setTitle:@"LiveCoding.TV"];
    
    [self initialize];
    [self request];
}


-(void)clearItems {
    [self.streamLiveItems removeAllObjects];
    [self.streamVideoItems removeAllObjects];
}
-(void)reload {
    
    [self clearItems];
    [self request];
}

-(void)request {
    
    if (self.viewMode == StreamListViewModeLive
        || self.viewMode == StreamListViewModeTopVideos) {
        AFHTTPRequestOperationManager* manager = [AFHTTPRequestOperationManager manager];
        manager.responseSerializer = [AFHTTPResponseSerializer serializer];
        
        [manager GET:HOST_NAME@"/livestreams/" parameters:nil success:^(AFHTTPRequestOperation *operation, id responseObject) {
            [self getStreamEntityList:responseObject];
            
        } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
            NSLog(@"Error: %@", error);
        }];
    } else {
        AFHTTPRequestOperationManager* manager = [AFHTTPRequestOperationManager manager];
        manager.responseSerializer = [AFHTTPResponseSerializer serializer];
        
        [manager GET:HOST_NAME@"/playlists/" parameters:nil success:^(AFHTTPRequestOperation *operation, id responseObject) {
            [self getStreamEntityList:responseObject];
            
        } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
            NSLog(@"Error: %@", error);
        }];

    }
}
- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


#pragma mark -
#pragma mark - UICollection Delegate

-(void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath {
    [self performSegueWithIdentifier:@"StreamPlayerViewController" sender:[[self streamLiveItems] objectAtIndex:[indexPath row]]];

}
#pragma mark - 
#pragma mark - UICollection Data Source

-(NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView {
    return 1;
}

-(NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
    if (self.viewMode == StreamListViewModeLive) {
        return [self.streamLiveItems count];
    } else {
        return [self.streamVideoItems count];
    }
    return 0;
}

-(UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath {
    StreamCollectionViewCell *cell = [collectionView dequeueReusableCellWithReuseIdentifier:@"StreamCollectionViewCell" forIndexPath:indexPath];
    
    if (!cell) {
        cell = [[StreamCollectionViewCell alloc] init];
    }
    
    cell.layer.borderColor = [[UIColor lightGrayColor] CGColor];
    cell.layer.borderWidth = 2.0f;
    cell.layer.cornerRadius = 5.0f;
    
    if (self.viewMode == StreamListViewModeLive) {
        cell.streamingEntity = [self.streamLiveItems objectAtIndex:indexPath.row];
    } else if (self.viewMode == StreamListViewModeTopVideos
               || self.viewMode == StreamListViewModePlayList) {
        cell.streamingEntity = [self.streamVideoItems objectAtIndex:indexPath.row];
    }

    
    return cell;
    
}

#pragma mark - 
#pragma mark - Public Methods
-(void)setViewMode:(StreamListViewMode)viewMode {
    _viewMode = viewMode;
    
    [self.collectionView reloadData];
}
#pragma mark -
#pragma mark - Private Methods

-(IBAction)onMenuButtonClicked:(id)sender {
    [self performSegueWithIdentifier:@"MenuViewController" sender:nil];
}

-(IBAction)onLeftButtonClicked:(id)sender {
    [self showLeftPanelIfNeed];
}
#pragma mark -
#pragma mark - UITableView Delegate

//-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
//    [self performSegueWithIdentifier:@"StreamPlayerViewController" sender:[[self streamItems] objectAtIndex:[indexPath row]]];
//}
//
//#pragma mark - 
//#pragma mark - UITableView DataSource
//
//-(NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
//    return 1;
//}
//
//-(NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
//    return [[self streamItems] count];
//}
//
//-(UITableViewCell*)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
//    
//    StreamEntityTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"StreamEntityTableViewCell"];
//    
//    [cell setStreamingEntity:[[self streamItems] objectAtIndex:[indexPath row]]];
//    return cell;
//}

#pragma mark -
#pragma mark - Segment Controller
-(void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    if ([[segue identifier] isEqualToString:@"StreamPlayerViewController"]) {
        StreamingEntity *entity = (StreamingEntity *)sender;
        StreamPlayerViewController *vc = [segue destinationViewController];
        [vc setEntity:entity];
    }
}
@end
