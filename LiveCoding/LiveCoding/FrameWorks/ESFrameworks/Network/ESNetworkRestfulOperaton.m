//
//  ESNetworkRestfulOperaton.m
//  Network
//
//  Created by Daehyun Kim on 2014. 6. 9..
//  Copyright (c) 2014년 Daehyun Kim. All rights reserved.
//

#import "ESNetworkRestfulOperaton.h"

@implementation ESNetworkRestfulOperaton

-(id)init {
    if (self = [super init]) {
        
        self.retryCount = 0;
        self.response = nil;
        self.error = nil;
        self.ref = nil;

    }
    return self;
}

-(void)main {
    self.response = nil;
    self.error = nil;
    
    NSURLResponse *response = nil;
    NSError *error = nil;
    
    NSURL *url = [[NSURL alloc] initWithString:self.url];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url cachePolicy:NSURLRequestUseProtocolCachePolicy timeoutInterval:15.0];
    [request setHTTPMethod:[self getHttpMethod:self.method]];
//    [NSMutableURLRequest setAllowsAnyHTTPSCertificate:YES forHost:[url host]];
    [request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"content-type"];

    if (self.params != nil) {
        if (self.method == ESNETWORK_RESTFUL_GET) {
            NSString *urlString = [NSString new];
            
            for (NSString *str in [self.params allKeys]) {
                urlString = [urlString stringByAppendingFormat:@"%@=",str];
                urlString = [urlString stringByAppendingString:[self.params objectForKey:str]];
                urlString = [urlString stringByAppendingString:@"&"];
            }
            urlString = [urlString substringToIndex:[urlString length]-1];
            [request setHTTPBody:[urlString dataUsingEncoding:NSUTF8StringEncoding]];
        } else {
            NSData *requestData = [[self requestData:self.params] dataUsingEncoding:NSUTF8StringEncoding];

            [request setHTTPBody:requestData];
        }
    } else {
    }
    
    NSData *ret = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error];
    
    NSString *str = [[NSString alloc] initWithData:ret encoding:NSUTF8StringEncoding];
    NSLog(@"str : %@", str);
    if (ret != nil) {
        NSError *jsonError = nil;
        NSObject *retDic = [NSJSONSerialization JSONObjectWithData:ret options:NSJSONReadingMutableContainers error:&jsonError];
        
        if (jsonError == nil && retDic != nil && ([retDic isKindOfClass:[NSDictionary class]] || [retDic isKindOfClass:[NSArray class]])) {
            self.result = retDic;
        } else {
            self.result = ret;
        }
    }
    
    self.response = response;
    self.error = error;
    self.retryCount++;
    
    [self.manager performSelectorOnMainThread:self.action withObject:self waitUntilDone:NO];

}

-(NSString *)getHttpMethod:(ESNETWORK_RESTFUL_METHOD) method {
    
    NSString *ret = @"";
    switch (method) {
        case ESNETWORK_RESTFUL_GET:
            ret = @"GET";
            break;
        case ESNETWORK_RESTFUL_POST:
            ret = @"POST";
            break;
        case ESNETWORK_RESTFUL_PUT:
            ret = @"PUT";
            break;
        case ESNETWORK_RESTFUL_DELETE:
            ret = @"DELETE";
            break;
            
        default:
            ret = @"GET";
            break;
    }
    return ret;
}

-(NSString *)requestData:(NSDictionary *)dic {
    NSMutableString *str = [[NSMutableString alloc] init];
    for (NSString* key in [dic allKeys]){
        if ([str length]>0)
            [str appendString:@"&"];
        [str appendFormat:@"%@=%@", key, [dic objectForKey:key]];
    }
    return str;
}

@end
