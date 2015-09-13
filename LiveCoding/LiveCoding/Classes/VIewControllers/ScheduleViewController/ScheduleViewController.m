//
//  ScheduleViewController.m
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 13..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "ScheduleViewController.h"
#import "ScheduleTableViewCell.h"

@implementation OptionClass

@end

@implementation ScheduleViewController

-(void)getScheduleList:(NSData *)data {
    TFHpple *xpath = [[TFHpple alloc] initWithData:data
                                             isXML:NO];
    NSArray *elements = [NSArray array];
    
    elements = [xpath searchWithXPathQuery:@"//html//body//section//section//div//section//div//ul[@class='schedule-list']//li"]; // <-- tags
    
    for (TFHppleElement *element in elements) {
        ScheduleEntity *entity = [[ScheduleEntity alloc] init];
        TFHppleElement *timeElement = [element searchWithXPathQuery:@"//div[@class='schedule-list-item--time']//span"][0];
        entity.time = [timeElement content];
        
        TFHppleElement *titleElement = [element searchWithXPathQuery:@"//a[@class='title']"][0];
        entity.title = [titleElement content];
        
        TFHppleElement *authorElement = [element searchWithXPathQuery:@"//div[@class='username']"][0];
        entity.author = [authorElement content];
        
        TFHppleElement *languageElement = [element searchWithXPathQuery:@"//div[@class='coding-category']"][0];
        entity.language = [languageElement content];
        
        //        TFHppleElement *languageElement = [element searchWithXPathQuery:@"//div[@class='coding-category']"][0];
        //        entity.language = [languageElement content];
        
        if ([element.attributes[@"class"] containsString:@" live online "]) {
            entity.isLive = YES;
        } else {
            entity.isLive = NO;
        }
        
        [self.scheduleItems addObject:entity];
        
    }
    
    
    if ([self.locationOptions count] <= 0) {
        TFHppleElement *timeZoneSelect = [xpath searchWithXPathQuery:@"//html//body//section//section//section//div//div//select[@id='timezone']"][0]; // <-- timezone
        NSArray *timezoneOptionList = [timeZoneSelect searchWithXPathQuery:@"//option"];
        for (TFHppleElement *element in timezoneOptionList) {
            OptionClass *opt = [[OptionClass alloc] init];
            
            opt.key = [element content];
            opt.value = element.attributes[@"value"];
            
            [self.locationOptions addObject:opt];
        }
    }
    
    TFHppleElement *codeCategorySelect = [xpath searchWithXPathQuery:@"//html//body//section//section//section//div//div//select[@id='coding-category']"][0]; // <-- timezone
    
    if ([self.codingCategoryOptions count] <= 0) {
        NSArray *codingCategoryOptionList = [codeCategorySelect searchWithXPathQuery:@"//option"];
        for (TFHppleElement *element in codingCategoryOptionList) {
            OptionClass *opt = [[OptionClass alloc] init];
            
            opt.key = [element content];
            opt.value = element.attributes[@"value"];
            
            [self.codingCategoryOptions addObject:opt];
        }
    }
    
    [self endRefreshControl];
    [self.tableView reloadData];
    
}


- (void)viewDidLoad {
    [super viewDidLoad];
    
    NSDate *date = [NSDate date];
    
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd"];
    
    self.currentDateString = [dateFormatter stringFromDate:date];
    [self setTitle:self.currentDateString];
    self.datePickerView.datePickerMode = UIDatePickerModeDate;
    
    self.scheduleItems = [NSMutableArray array];
    self.codingCategoryOptions = [NSMutableArray array];
    self.locationOptions = [NSMutableArray array];
    
    [self requestSchedule];
    
    __weak ScheduleViewController *weak = self;
    
    [self setRefreshWithScrollView:self.tableView withAction:^{
        [weak reload];
    }];
}

-(void)clearItems {
    [self.scheduleItems removeAllObjects];
}

-(void)reload {
    [self clearItems];
    
    [self setTitle:self.currentDateString];
    if (self.currentType == ScheduleOptionTypeTimezone) {
        [self requestSetTimeZone];
    } else {
        [self requestSchedule];
    }
}

-(void)requestSetTimeZone {
    AFHTTPRequestOperationManager* manager = [AFHTTPRequestOperationManager manager];
    manager.responseSerializer = [AFHTTPResponseSerializer serializer];
    
    if (self.selectedLocationOption) {
        [manager GET:@"http://livecoding.tv/accounts/set-timezone" parameters:@{@"timezone":self.selectedLocationOption.key} success:^(AFHTTPRequestOperation *operation, id responseObject) {
            [self requestSchedule];
        } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
            NSLog(@"Error: %@", error);
        }];
    } else {
        [self requestSchedule];
    }
}
-(void)requestSchedule {
    
    AFHTTPRequestOperationManager* manager = [AFHTTPRequestOperationManager manager];
    manager.responseSerializer = [AFHTTPResponseSerializer serializer];
    
    NSString *url = [NSString stringWithFormat:@"%@/schedule/%@", HOST_NAME, self.currentDateString];
    
    if (self.selectedCodingCategoryOption) {
        url = [url stringByAppendingFormat:@"?category=%@", [self.selectedCodingCategoryOption value]];
    }
    
    [manager GET:url parameters:nil success:^(AFHTTPRequestOperation *operation, id responseObject) {
        [self getScheduleList:responseObject];
    } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
        NSLog(@"Error: %@", error);
    }];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark -
#pragma mark - Private Methods

-(IBAction)onDateButtonClicked:(id)sender {
    self.currentType = ScheduleOptionTypeDate;
    [self showPickerView];
}

-(IBAction)onLocationButtonClicked:(id)sender {
    self.currentType = ScheduleOptionTypeTimezone;
    [self.pickerView selectRow:0 inComponent:0 animated:NO];
    [self.pickerView reloadAllComponents];
    [self showPickerView];
}

-(IBAction)onCategoryButtonClicked:(id)sender {
    self.currentType = ScheduleOptionTypeCodingCategory;
    [self.pickerView selectRow:0 inComponent:0 animated:NO];
    [self.pickerView reloadAllComponents];
    [self showPickerView];
}

-(IBAction)onOkButtonClicked:(id)sender {
    
    if (self.currentType == ScheduleOptionTypeDate) {
        NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
        [dateFormatter setDateFormat:@"yyyy-MM-dd"];
        self.currentDateString = [dateFormatter stringFromDate:self.datePickerView.date];
    }
    
    [self reload];
    [self hiddenPickerView];
    
    self.currentType = ScheduleOptionTypeNone;
}


-(IBAction)onCancelButtonClicked:(id)sender {
    [self hiddenPickerView];
}

-(void)showPickerView {
    
    if (self.currentType == ScheduleOptionTypeDate) {
        [self.datePickerView setHidden:NO];
        [self.pickerView setHidden:YES];
    } else {
        [self.datePickerView setHidden:YES];
        [self.pickerView setHidden:NO];
    }
    
    
    [self.pickerBackgroundView setHidden:NO];
    [self.pickerBackgroundView setAlpha:0.0f];
    [UIView animateWithDuration:0.3f animations:^{
        [self.pickerBackgroundView setAlpha:1.0f];
    } completion:^(BOOL finished) {
        
    }];
}

-(void)hiddenPickerView {
    [self.pickerBackgroundView setAlpha:1.0f];
    [UIView animateWithDuration:0.3f animations:^{
        [self.pickerBackgroundView setAlpha:0.0f];
    } completion:^(BOOL finished) {
        [self.pickerBackgroundView setHidden:YES];
    }];
}


#pragma mark -
#pragma mark - UIPicker Delegate

-(void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component {
    if (self.currentType == ScheduleOptionTypeCodingCategory) {
        self.selectedCodingCategoryOption = [self.codingCategoryOptions objectAtIndex:row];
    } else {
        self.selectedLocationOption = [self.locationOptions objectAtIndex:row];
    }
    
}
#pragma mark -
#pragma mark - UIPickerView DataSource

-(NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView {
    return 1;
}

-(NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component {
    if (self.currentType == ScheduleOptionTypeCodingCategory) {
        return [self.codingCategoryOptions count];
    } else if (self.currentType == ScheduleOptionTypeTimezone) {
        return [self.locationOptions count];
    }
    return 0;
}

-(NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component {
    
    OptionClass *opt;
    if (self.currentType == ScheduleOptionTypeCodingCategory) {
        opt = [self.codingCategoryOptions objectAtIndex:row];
    } else if (self.currentType == ScheduleOptionTypeTimezone) {
        opt = [self.locationOptions objectAtIndex:row];
    }
    
    return [opt key];
}
#pragma mark -
#pragma mark - UITableView Delegate

-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    
}
#pragma mark -
#pragma mark - UITableView DataSource

-(NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

-(NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [self.scheduleItems count];
}

-(UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    ScheduleTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ScheduleTableViewCell"];
    
    [cell setScheduleEntity:[self.scheduleItems objectAtIndex:indexPath.row]];
    return cell;
}
@end
