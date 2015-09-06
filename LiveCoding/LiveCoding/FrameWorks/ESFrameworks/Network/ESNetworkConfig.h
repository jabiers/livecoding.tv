//
//  ESNetworkConfig.h
//  Network
//
//  Created by Daehyun Kim on 2014. 6. 9..
//  Copyright (c) 2014년 Daehyun Kim. All rights reserved.
//

#ifndef Network_ESNetworkConfig_h
#define Network_ESNetworkConfig_h

#define MAX_CONCURRENT_OPERATION_COOUNT 5   // 한번에 실행할 수 있는 오퍼레이션 수
#define AVERAGE_REQUEST_WAIT_TIME       1  // 리퀘스트 하나에 대해서 10초 기다려 줌
#define MAX_RETRY_COUNT                 2

typedef enum {
    ESNETWORK_RESTFUL_GET = 0,
    ESNETWORK_RESTFUL_PUT,
    ESNETWORK_RESTFUL_POST,
    ESNETWORK_RESTFUL_DELETE,
    
} ESNETWORK_RESTFUL_METHOD;

#endif
