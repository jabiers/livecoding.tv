//
//  StreamListViewController.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 5..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "StreamListViewController.h"
#import "StreamEntityTableViewCell.h"

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
   
    elements = [xpath searchWithXPathQuery:@"//html//body//div"]; // <-- tags
    
    self.streamItems = [NSMutableArray array];
    
    for (TFHppleElement *element in elements) {
        
        if ([element.attributes[@"class"] isEqualToString:@"small-video-box"]) {
            StreamingEntity *entity = [[StreamingEntity alloc] init];
            for (TFHppleElement *child in element.children) {
                NSLog(@"child class : %@", child);
                if ([child.tagName isEqualToString:@"a"]) {
                    entity.streamingUrl = [NSString stringWithFormat:@"%@%@",HOST_NAME, child.attributes[@"href"]];
                    TFHppleElement *img = child.children[3];
                    entity.thumbUrl = [HOST_NAME stringByAppendingString:img.attributes[@"src"]];
                    entity.author = [child.attributes[@"href"] substringWithRange:NSMakeRange(1, [child.attributes[@"href"] length] - 2)];
                    entity.title = img.attributes[@"alt"];
                }
            }
            
            [self.streamItems addObject:entity];
        }
    }
    
    
    for (StreamingEntity *entity in self.streamItems) {
        NSLog(@"\ntitle : %@\nauthor : %@\nentity url : %@\n thumb url : %@", entity.title,entity.author, entity.streamingUrl, entity.thumbUrl);
    }
    
    [self.tableView reloadData];
    
//    NSArray *sourceArr = [doc searchWithXPathQuery:@"<div class=\"small-video-box\">"];
    
   
/*
    <div class="small-video-box">
    <a class="item woopra_live_click" href="/paulkim/"><div class="playBtnbg"></div><span class="playBtn"></span>
				
    <img class="thumbnail thumbnail-250x140" src="/video/livestream/paulkim/thumbnail_250_140/" alt="make livecoding.tv App for iOS (then Android)" />
				
    </a>
    <span class="white-span"><a class="woopra_live_click" href="/paulkim/" title="make livecoding.tv App for iOS (then Android)" data-toggle="tooltip" data-placement="bottom">make livecoding.tv App fo...</a></span>
    <span>
				<img src="/static/flags/kr.gif" class="country-flag" alt="South Korea" data-title="South Korea" data-toggle="tooltip" data-placement="right" />&nbsp;PAULKIM
    </span>
    <span class="info text-ellipsis">
				
    <span class="item" title="Views" data-toggle="tooltip" data-placement="right"><i class="fa fa-user"></i>&nbsp;14</span>
    <span class="item">Obj-C/Swift (iOS)</span>
    <span class="item">(intermediate)</span>
    </span>
    </div>
*/
    // Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


#pragma mark - 
#pragma mark - Private Methods 

-(IBAction)onMenuButtonClicked:(id)sender {
    [self performSegueWithIdentifier:@"MenuViewController" sender:nil];
}
#pragma mark -
#pragma mark - UITableView Delegate

-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [self performSegueWithIdentifier:@"StreamPlayerViewController" sender:[[self streamItems] objectAtIndex:[indexPath row]]];
}

#pragma mark - 
#pragma mark - UITableView DataSource

-(NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

-(NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [[self streamItems] count];
}

-(UITableViewCell*)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    StreamEntityTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"StreamEntityTableViewCell"];
    
    [cell setStreamingEntity:[[self streamItems] objectAtIndex:[indexPath row]]];
    return cell;
}

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
