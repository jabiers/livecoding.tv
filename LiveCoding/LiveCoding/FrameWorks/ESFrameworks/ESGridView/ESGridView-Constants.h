//
//  ESGridView-Constants.h
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 27..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#ifndef gridview_ESGridView_Constants_h
#define gridview_ESGridView_Constants_h


//
// ARC on iOS 4 and 5
//

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_5_0 && !defined (ES_DONT_USE_ARC_WEAK_FEATURE)

#define es_weak   weak
#define __es_weak __weak
#define es_nil(x)


#else

#define es_weak   unsafe_unretained
#define __es_weak __unsafe_unretained
#define es_nil(x) x = nil

#endif


//
// Code specific
//

#define INVALID_POSITION -1

#define kTagOffset 50
#define kDefaultSize CGSizeMake(170,124)

#endif
