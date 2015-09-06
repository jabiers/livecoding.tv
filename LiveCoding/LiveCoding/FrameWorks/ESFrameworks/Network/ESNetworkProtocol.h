//
//  ESNetworkProtocol.h
//  Network
//
//  Created by Daehyun Kim on 2014. 6. 9..
//  Copyright (c) 2014년 Daehyun Kim. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ESNetworkConfig.h"
#import "ESNetworkReceiveProtocol.h"

@protocol ESNetworkProtocol <NSObject>

-(void)sendRequestRestful:(ESNETWORK_RESTFUL_METHOD) method
                  withUrl:(NSString *) url
               withParams:(NSDictionary *) params
               withTarget:(id<ESNetworkReceiveProtocol>) target;

-(void)sendRequestRestful:(ESNETWORK_RESTFUL_METHOD) method
                  withUrl:(NSString *) url
               withParams:(NSDictionary *) params
               withTarget:(id<ESNetworkReceiveProtocol>)target
                  withRef:(id)ref;

-(void)sendRequestRestful:(ESNETWORK_RESTFUL_METHOD) method
                  withUrl:(NSString *) url
               withParams:(NSDictionary *) params
               withTarget:(id<ESNetworkReceiveProtocol>) target
       willShowRetryAlert:(BOOL)willShow
                  withRef:(id)ref;

@end
