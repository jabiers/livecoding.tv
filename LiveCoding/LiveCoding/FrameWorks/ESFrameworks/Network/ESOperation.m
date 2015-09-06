//
//  ESOperation.m
//  Network
//
//  Created by Daehyun Kim on 2014. 6. 9..
//  Copyright (c) 2014년 Daehyun Kim. All rights reserved.
//

#import "ESOperation.h"

@implementation ESOperation 


- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace{
    return YES;
}
-(void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
    
}

@end
