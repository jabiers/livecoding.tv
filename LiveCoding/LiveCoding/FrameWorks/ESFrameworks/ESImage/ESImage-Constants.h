//
//  CuImage-Constants.h
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 29..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#ifndef gridview_ESImage_Constants_h
#define gridview_ESImage_Constants_h

#define COMMON_JPEGCOMPRESSIONQULITY 0.5f
#define JPEG_compressionQuality COMMON_JPEGCOMPRESSIONQULITY
#define DEFAULT_IMAGE_FORMAT ESImageFormatJPEG
#define DEFAULT_IMAGE_CACHE YES
#define DEFAULT_IMAGETYPE ESImageTypeOriginal
#define DEFAULT_RESIZE_QULITY (CGInterpolationQuality)ESImageResizeQulityDefault
#define MAX_CONCURRENT_OPERATION_COOUNT 5
#define MAX_CACHEIMAGE_COUNT 100
#define DELETED_IMAGE_COUNT_ONETIME 10

#define MAX_ORIGINALIMAGE_CACHECOUNT 30
#define DELETED_ORIGINALIMAGE_COUNT_ONETIME 3

typedef enum {
    ESImageTypeOriginal = 0,
    ESImageTypeResize50Percent,
    ESImageTypeResize25Percent,
    ESImageTypeResizeThumbNail,
    ESImageTypeLimitedOriginal,
    ESImageTypeSpecificalSize
} ESImageType;

typedef enum CGInterpolationQuality ESImageResizeQulity;

enum ESImageResizeQulity {
    ESImageResizeQulityDefault = 0,	/* Let the context decide. */
    ESImageResizeQulityNone = 1,		/* Never interpolate. */
    ESImageResizeQulityLow = 2,		/* Low quality, fast interpolation. */
    ESImageResizeQulityMedium = 4,		/* Medium quality, slower than kCGInterpolationLow. */
    ESImageResizeQulityHigh = 3		/* Highest quality, slower than kCGInterpolationMedium. */
};

typedef struct {
    __unsafe_unretained NSString *imageUrl;
    ESImageType imageType;
    ESImageResizeQulity resizeQulity;
    BOOL isImageCache;
} ESImageLoadConfig;


#endif
