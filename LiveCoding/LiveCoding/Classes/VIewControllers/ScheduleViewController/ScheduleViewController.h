//
//  ScheduleViewController.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 13..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import "ESViewController.h"

@interface OptionClass : NSObject

@property (strong, nonatomic) NSString *key;
@property (strong, nonatomic) NSString *value;
@end

typedef enum {
    ScheduleOptionTypeNone,
    ScheduleOptionTypeDate,
    ScheduleOptionTypeTimezone,
    ScheduleOptionTypeCodingCategory
}ScheduleOptionType;

@interface ScheduleViewController : ESViewController <UITableViewDataSource, UITableViewDelegate
,UIPickerViewDataSource, UIPickerViewDelegate>

@property (weak, nonatomic) IBOutlet UITableView *tableView;
@property (strong, nonatomic) NSMutableArray *scheduleItems;

@property (weak, nonatomic) IBOutlet UIButton *dateButton;
@property (weak, nonatomic) IBOutlet UIButton *locationButton;
@property (weak, nonatomic) IBOutlet UIButton *categoryButton;
@property (strong, nonatomic) NSMutableArray *locationOptions;
@property (strong, nonatomic) NSMutableArray *codingCategoryOptions;
@property (assign, nonatomic) ScheduleOptionType currentType;

@property (strong, nonatomic) NSString *currentDateString;
@property (strong, nonatomic) OptionClass *selectedCodingCategoryOption;
@property (strong, nonatomic) OptionClass *selectedLocationOption;

@property (weak, nonatomic) IBOutlet UIView *pickerBackgroundView;
@property (weak, nonatomic) IBOutlet UIDatePicker *datePickerView;
@property (weak, nonatomic) IBOutlet UIPickerView *pickerView;
@property (weak, nonatomic) IBOutlet UIButton *cancelButton;
@property (weak, nonatomic) IBOutlet UIButton *okButton;

@end
