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

@implementation StreamListViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [self setTitle:@"LiveCoding.TV"];
//    [self.network sendRequestRestful:ESNETWORK_RESTFUL_GET withUrl:HOSTNAME[@"/livestreams/" withParams:nil withTarget:self]];
    
    NSURL *url = [NSURL URLWithString:HOST_NAME@"/livestreams/"];
    NSData *data = [NSData dataWithContentsOfURL:url];
    NSString *sourceString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    
    TFHpple *xpath = [[TFHpple alloc] initWithData:data isXML:NO];
    NSArray *elements = [NSArray array];
   
    elements = [xpath searchWithXPathQuery:@"//html//body//div[@class='browse-main-videos--item']"]; // <-- tags
        
    self.streamItems = [NSMutableArray array];
    
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
            entity.contry = [obj attributes][@"src"];
        }
        
        TFHppleElement *nameElement = [element searchWithXPathQuery:@"//span"][2];
        NSString *author = [nameElement content];
        
        author = [author stringByReplacingOccurrencesOfString:@"\t" withString:@""];
        author = [author stringByReplacingOccurrencesOfString:@"\n" withString:@""];
        author = [[author componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] componentsJoinedByString:@""];
        
        entity.author = author;
        
        NSArray *item = [element searchWithXPathQuery:@"//span[@class='browse-main-videos--info']"];
        
        TFHppleElement *video_info = item[0];
        
        NSArray *expertElement = [video_info searchWithXPathQuery:@"//span[@class='browse-main-videos--info-item']"];
        
        entity.expert = [[[expertElement[0] content]componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] componentsJoinedByString:@""];
        
//        NSLog(@"expert : %@", expert);
        
        entity.language = [expertElement[1] content];
//        dic[@"expert"] = [item[2] content];
        
        [self.streamItems addObject:entity];
    }
    
    
    [self.collectionView reloadData];
//    [self.tableView reloadData];
    
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


#pragma mark -
#pragma mark - UICollection Delegate

-(void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath {
    [self performSegueWithIdentifier:@"StreamPlayerViewController" sender:[[self streamItems] objectAtIndex:[indexPath row]]];

}
#pragma mark - 
#pragma mark - UICollection Data Source

-(NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView {
    return 1;
}

-(NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
    return [self.streamItems count];
}

-(UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath {
    StreamCollectionViewCell *cell = [collectionView dequeueReusableCellWithReuseIdentifier:@"StreamCollectionViewCell" forIndexPath:indexPath];
    
    if (!cell) {
        cell = [[StreamCollectionViewCell alloc] init];
    }
    cell.streamingEntity = [self.streamItems objectAtIndex:indexPath.row];
    
    return cell;
    
}

#pragma mark -
#pragma mark - Private Methods

-(IBAction)onMenuButtonClicked:(id)sender {
    [self performSegueWithIdentifier:@"MenuViewController" sender:nil];
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
#pragma mark - ESNetworkReceive Delegate
-(void)didReceiveRequest:(NSString *)url withResult:(id)result withError:(NSError *)error withRef:(id)ref {
    
}

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
