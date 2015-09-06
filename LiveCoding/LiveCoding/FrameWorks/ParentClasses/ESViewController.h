//
//  ESViewController.h
//  LiveCoding
//
//  Created by Kim DaeHyun on 2015. 9. 6..
//  Copyright (c) 2015년 Kim DaeHyun. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ESNetwork.h"

@interface ESViewController : UIViewController <ESNetworkReceiveProtocol>

@property (strong, nonatomic) ESNetwork *network;

@end
