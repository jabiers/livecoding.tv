//
//  ESOperation.h
//  Network
//
//  Created by Daehyun Kim on 2014. 6. 9..
//  Copyright (c) 2014년 Daehyun Kim. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ESNetworkConfig.h"
#import "ESNetworkReceiveProtocol.h"

@interface NSMutableURLRequest (DummyInterface)
+(BOOL)allowsAnyHTTPSCertificateForHost:(NSString *)host;
//+(void)setAllowsAnyHTTPSCertificate:(BOOL)allow forHost:(NSString *)host;
@end

@interface ESOperation : NSOperation <NSURLConnectionDataDelegate>

@property (nonatomic, weak) id<ESNetworkReceiveProtocol> target;
@property (nonatomic, weak) id ref;

@property (nonatomic, strong) id manager;
@property (nonatomic, assign) SEL action;
@property (nonatomic, assign) BOOL willShowAlert;

@property (nonatomic, strong) NSString *url;
@property (nonatomic, assign) ESNETWORK_RESTFUL_METHOD method;
@property (nonatomic, strong) NSDictionary *params;
@property (nonatomic, assign) NSInteger retryCount;

@property (nonatomic, strong) id result;
@property (nonatomic, strong) NSURLResponse *response;
@property (nonatomic, strong) NSError *error;

@end
