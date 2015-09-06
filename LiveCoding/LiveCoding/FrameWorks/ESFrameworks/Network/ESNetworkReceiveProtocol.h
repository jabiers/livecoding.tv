//
//  ESNetworkReceiveProtocol.h
//  Network
//
//  Created by Daehyun Kim on 2014. 6. 9..
//  Copyright (c) 2014년 Daehyun Kim. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol ESNetworkReceiveProtocol <NSObject>

-(void)didReceiveRequest:(NSString *) url withResult:(id)result withError:(NSError *)error withRef:(id)ref;

@end