#import "ImageManager.h"
#import "ImageOperation.h"
#import "ESImageView.h"
#import <malloc/malloc.h>
#import <objc/runtime.h>
#import "ESImage.h"
//#import "ESPDFViewWithZoom.h"

@implementation CacheImage

-(NSString *)description {
    
    return [NSString stringWithFormat:@"imageUrl : %@, qulity : %d, size : %@, imageType : %d, refCount :%d", self.imageUrl, self.qulity, NSStringFromCGSize(self.size), self.imageType, self.refCount];
}

-(UIImage *)image {
    return [UIImage imageWithData:self.imageData];
}

-(id)self {
    return (id)self.image;
}
@end

@interface ImageManager ()

@end

@implementation ImageManager
@synthesize cacheImagesList;

#pragma mark - Singleton
static ImageManager *sharedInstance;
+(ImageManager *)shareInstance {
    
    if (!sharedInstance) {
        sharedInstance = [[ImageManager alloc] init];
    }
    
    return sharedInstance;
}


#pragma mark - Initialize

-(id)init {
    if (self = [super init]) {
        
        [self initialize];
    }
    return self;
}

-(void)initialize {
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(handleMemoryWarning:) name:UIApplicationDidReceiveMemoryWarningNotification object:nil];
    self.operationQueue = [[NSOperationQueue alloc] init];
    [self.operationQueue setMaxConcurrentOperationCount:MAX_CONCURRENT_OPERATION_COOUNT];
    cacheImages = [[NSMutableDictionary alloc] init];
    cacheImagesIndex = [[NSMutableArray alloc] init];
    cacheImagesList = [[NSMutableArray alloc] init];
    
    
    self.test = NO;
    //    originalImages = [[NSMutableDictionary alloc] init];
    //    originalImagesIndex = [[NSMutableArray alloc] init];
}

-(void)handleMemoryWarning:(id)sender {
    [cacheImagesList sortUsingComparator:^NSComparisonResult(id obj1, id obj2) {
        NSComparisonResult result = NSOrderedSame;
        if(((CacheImage*)obj1).refCount < ((CacheImage*)obj2).refCount)
            result = NSOrderedAscending;
        else
            result = NSOrderedDescending;
        return result;
    }];
    
    for (int i = 0; i < [cacheImagesList count] / 2; i ++) {
        [cacheImagesList removeObjectAtIndex:0];
    }
}


#pragma mark - cache methods

-(void)addCacheImageWithImageUrl:(NSString *)imageUrl withContext:(id)context {
    [self getImageUrl:imageUrl withTarget:self withType:ESImageTypeOriginal isImageCache:YES withContext:context];
}

#pragma mark - getters Image with ImageUrl

-(BOOL)getImageUrl:(NSString *)imageUrl
            withTarget:(id)target
           withContext:(id)context {
    
    return [self getImageUrl:imageUrl
                      withTarget:target
                        withType:DEFAULT_IMAGETYPE
                     withContext:context];
}

-(BOOL)getImageUrl:(NSString *)imageUrl
            withTarget:(id)target
              withType:(ESImageType)imageType
           withContext:(id)context{
    
    return [self getImageUrl:imageUrl
                      withTarget:target
                        withType:imageType
                    isImageCache:DEFAULT_IMAGE_CACHE
                     withContext:context];
}

-(BOOL)getImageUrl:(NSString *)imageUrl
            withTarget:(id)target
              withSize:(CGSize)size
           withContext:(id)context{
    
    return [self getImageUrl:imageUrl
                      withTarget:target
                        withSize:size
                    isImageCache:DEFAULT_IMAGE_CACHE
                     withContext:context];
}

-(BOOL)getImageUrl:(NSString*)imageUrl
            withTarget:(id)target
              withType:(ESImageType)imageType
          isImageCache:(BOOL)isImageCache
           withContext:(id)context {
    
    return [self getImageUrl:imageUrl
                      withTarget:target
                        withType:imageType
                        withSize:CGSizeZero
                    isImageCache:isImageCache withContext:context];
}

-(BOOL)getImageUrl:(NSString*)imageUrl
            withTarget:(id)target
              withSize:(CGSize)size
          isImageCache:(BOOL)isImageCache
           withContext:(id)context{
    
    return [self getImageUrl:imageUrl
                      withTarget:target
                        withType:ESImageTypeSpecificalSize
                        withSize:size
                    isImageCache:isImageCache
                     withContext:context];
}

-(BOOL)getImageUrl:(NSString*)imageUrl
            withTarget:(id)target
              withType:(ESImageType)imageType
              withSize:(CGSize)size
          isImageCache:(BOOL)isImageCache
           withContext:(id)context{
    
    return [self getImageUrl:imageUrl
                      withTarget:target
                        withType:imageType
                        withSize:size
                withResizeQulity:DEFAULT_RESIZE_QULITY
                    isImageCache:isImageCache
                     withContext:context];
}

-(BOOL)getImageUrl:(NSString *)imageUrl
            withTarget:(id)target
              withType:(ESImageType)imageType
              withSize:(CGSize)size
      withResizeQulity:(ESImageResizeQulity)resizeQulity
          isImageCache:(BOOL)isImageCache
           withContext:(id)context {
    
    BOOL ret = YES;
    
    
    if (imageType == ESImageTypeLimitedOriginal && 3000 * 2000 < size.width * size.height) {
        CGFloat scale = ( size.width / 3000 ) > ( size.height / 2000 )? size.width / 3000 : size.height / 2000;
        size = CGSizeMake(size.width / scale, size.height / scale);

    }
    for (int i = 0; i < [cacheImagesList count]; i ++) {
        CacheImage *one = [cacheImagesList objectAtIndex:i];
        
        if (([one.imageUrl isEqualToString:imageUrl]
             && one.imageType == imageType
             && one.qulity == resizeQulity
             && (CGSizeEqualToSize(CGSizeMake(one.size.width / [[UIScreen mainScreen] scale] , one.size.height / [[UIScreen mainScreen] scale]), size)
                 || one.imageType == ESImageTypeOriginal))
            || (one.imageType == ESImageTypeLimitedOriginal
                && [one.imageUrl isEqualToString:imageUrl]
                && one.qulity == resizeQulity)) {
            
            if ([target respondsToSelector:@selector(imageManager:DidLoadImage:withContext:)]) {
                dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                    [NSThread sleepForTimeInterval:0.001];
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [target imageManager:self DidLoadImage:one withContext:context];
                    });
                });

            }
            
            one.refCount++;
            [cacheImagesList replaceObjectAtIndex:i withObject:one];
            
            return ret;
        }
    }
    ImageOperation *operation;
    
    if (imageType == ESImageTypeOriginal) {
        operation = [ImageOperation operationManager:self withAction:@selector(didEndOperationWithResult:withContext:) withTarget:target withUrl:imageUrl withImageOprationType:imageType withSize:size isImageCache:isImageCache withContext:context];
        
    } else {
        UIImage *oriImage;
        for (CacheImage *one in cacheImagesList) {
            if (one.imageType == ESImageTypeOriginal && [one.imageUrl isEqualToString:imageUrl]) {
                oriImage = one.image;
            }
        }
        operation = [ImageResizeOperation ResizeOperationManager:self withAction:@selector(didEndOperationWithResult:withContext:) withTarget:target withUrl:imageUrl withImageOprationType:imageType withOriginalImage:oriImage withSize:size withResizeQulity:resizeQulity isImageCache:isImageCache withContext:context];
    }
    
    [self.operationQueue addOperation:operation];
    
    return ret;
}



#pragma mark - Received Image
-(void)didEndOperationWithResult:(NSDictionary *)result withContext:(id)context {
    id <ImageManagerDelegate> target = [result objectForKey:@"!target"];
    
    if(result == nil && [target isEqual:[NSNull null]]) return;
    
    NSError *error = [result objectForKey:@"!error"];
    if ([target isKindOfClass:[NSString class]]) {
        
        for (int i = 0; i < [cacheImagesList count]; i++) {
            CacheImage *one = [cacheImagesList objectAtIndex:i];
            if ([one.imageUrl isEqualToString:[result objectForKey:@"!imageUrl"]] && CGSizeEqualToSize(one.size, [[result objectForKey:@"!imageSize"] CGSizeValue])) {
                one.imageData = [result objectForKey:@"!imageData"];
                [cacheImagesList replaceObjectAtIndex:i withObject:one];
            }
        }
    }
    
    if ([error isKindOfClass:[NSError class]]) {

        if ([target respondsToSelector:@selector(imageManager:didFailLoadImageUrl:withError:withContext:)]) {
            NSLog([result objectForKey:@"!imageUrl"], @"Failed to Load Image");
            [target imageManager:self didFailLoadImageUrl:[result objectForKey:@"!imageUrl"] withError:[result objectForKey:@"!error"] withContext:[result objectForKey:@"!context"]];
        }
        
    } else {
        
        ESImageType imageType = [[result objectForKey:@"!imageType"] intValue];
        ESImageResizeQulity resizeQulity = [[result objectForKey:@"!resizeQulity"] intValue];
        CGSize size = [[result objectForKey:@"!imageSize"] CGSizeValue];
        NSString *imageUrl = [result objectForKey:@"!imageUrl"];
        BOOL isImageCache = [[result objectForKey:@"!isImageCache"] boolValue];
        NSData *imageData = [result objectForKey:@"!imageData"];
        
        CacheImage *image = [[CacheImage alloc] init];
        
        if(imageData == nil || imageData.length == 0) {
            NSLog([result objectForKey:@"!imageUrl"], @"Failed to Load Image - iamge size is zero");
            if ([target respondsToSelector:@selector(imageManager:didFailLoadImageUrl:withError:withContext:)]) {
                NSLog([result objectForKey:@"!imageUrl"], @"Failed to Load Image");
                [target imageManager:self didFailLoadImageUrl:[result objectForKey:@"!imageUrl"] withError:[result objectForKey:@"!error"] withContext:[result objectForKey:@"!context"]];
            }

        }
        else {
            if (isImageCache) {
                [image setImageData:imageData];
                [image setImageType:imageType];
                [image setQulity:resizeQulity];
                [image setSize:size];
                [image setImageUrl:imageUrl];
                [image setRefCount:1];
                [cacheImagesList addObject:image];
            }
            else {
                [image setImageData:imageData];
                [image setImageType:imageType];
                [image setSize:size];
            }
            
            if ([target isKindOfClass:[ESImageView class]]) {
                ESImageView *temp = (ESImageView*)target;
                if (temp.imageUrl != imageUrl) {
                    return;
                }
            }
            
            if ([target respondsToSelector:@selector(imageManager:DidLoadImage:withContext:)]) {
                NSLog(imageUrl, @"Success to Load Image");
                [target imageManager:self DidLoadImage:image withContext:[result objectForKey:@"!context"]];
            }
        }
    }
}

-(void)removeAllOperation {
    
    
    [[NSUserDefaults standardUserDefaults] setValue:[NSDate date] forKey:@"OperationCancelTime"];
    
    //    [self.operationQueue cancelAllOperations];
    
    //    [self.operationQueue setSuspended:YES];
}

-(void)removeCacheImages {
    [cacheImagesList sortUsingComparator:^NSComparisonResult(id obj1, id obj2) {
        NSComparisonResult result = NSOrderedSame;
        if(((CacheImage*)obj1).refCount < ((CacheImage*)obj2).refCount)
            result = NSOrderedAscending;
        else
            result = NSOrderedDescending;
        return result;
    }];
    
    for (int i = 0; i < [cacheImagesList count] / 2; i ++) {
        [cacheImagesList removeObjectAtIndex:0];
    }
    
}

-(void)asynchronousImageLoadWithImageUrl:(NSString *)imageUrl
                               withBlock:(void(^)(NSData *imageData, NSError *error, BOOL success))block {

    
//    ImageOperationWithBlock *operation = [[ImageOperationWithBlock alloc] initWithManager:self withAction:@selector(didEndOperationWithResult:withContext:) withTarget:nil withUrl:imageUrl withImageOprationType:0 withSize:CGSizeZero isImageCache:YES withContext:nil];
//
//    [operation performSelector:@selector(blockTest:) withObject:block afterDelay:5];

    for (int i = 0; i < [cacheImagesList count]; i ++) {
        CacheImage *one = [cacheImagesList objectAtIndex:i];
        
        if ([one.imageUrl isEqualToString:imageUrl]
            && one.imageType == ESImageTypeOriginal ) {

            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [NSThread sleepForTimeInterval:0.001];
                dispatch_async(dispatch_get_main_queue(), ^{
                    block(one.imageData, nil, one.imageData?YES:NO);
                });
            });
            one.refCount++;
            [cacheImagesList replaceObjectAtIndex:i withObject:one];
            return;
        }
    }
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        if (imageUrl) {
            NSLog(imageUrl, @"Image Get URL");

            NSURL *url = [NSURL URLWithString:imageUrl];
            
            NSError *error;
            NSData *imageData = [NSData dataWithContentsOfURL:url options:NSDataReadingMappedAlways error:&error];
#ifdef APP_OPTION_AutoConversionPDFInManager
            imageData = [self checkAndMakePngFromPDF:imageData];
#endif
            
            dispatch_async(dispatch_get_main_queue(), ^{
                CacheImage *image = [[CacheImage alloc] init];
                
                [image setImageData:imageData];
                [image setImageType:ESImageTypeOriginal];
                [image setQulity:DEFAULT_RESIZE_QULITY];
                [image setSize:CGSizeMake(0, 0)];
                [image setImageUrl:imageUrl];
                [image setRefCount:1];
                [cacheImagesList addObject:image];
                
                block(image.imageData, nil, image.imageData?YES:NO);
            });
        }
    });
}

//- (NSData *)checkAndMakePngFromPDF:(NSData *)data
//{
//    NSData *imageData = data;
//    ESImageFormat format = [ESImage imageFormatForImageData:imageData];
//    if(format == ESImageFormatPDF) {
//        UIImage *tempImage = [ESPDFViewWithZoom createPageImageWithData:imageData withSize:CGSizeZero at:1];
//        imageData = UIImagePNGRepresentation(tempImage);
//    }
//    return imageData;
//}

-(CacheImage*)synchronousImageLoadWithImageUrl:(NSString *)imageUrl
                                      withSize:(CGSize)size
                                      withType:(ESImageType)imageType
                              withResizeQulity:(ESImageResizeQulity)resizeQulity
                                  isImageCache:(BOOL)isImageCache {
    
    NSURL *url = [NSURL URLWithString:imageUrl];
    
    NSData *imageData;
    NSError *error;
    if (imageUrl) {
        imageData = [NSData dataWithContentsOfURL:url options:NSDataReadingMappedAlways error:&error];
#ifdef APP_OPTION_AutoConversionPDFInManager
        imageData = [self checkAndMakePngFromPDF:imageData];
#endif
    }
    
    CacheImage *image = [[CacheImage alloc] init];
    
    [image setImageData:imageData];
    [image setImageType:imageType];
    [image setQulity:resizeQulity];
    [image setSize:size];
    [image setImageUrl:imageUrl];
    [image setRefCount:1];
    
    if (isImageCache) {
        [cacheImagesList addObject:image];
    }
    
    return image;
}

-(void)replaceCacheImage:(ESImage *)image forImageUrl:(NSString *)imageUrl {
    
    for (int i = 0; i < [cacheImagesList count]; i++) {
        CacheImage *one = [cacheImagesList objectAtIndex:i];
        
        if ([one.imageUrl isEqualToString:imageUrl]) {
            if (one.imageType == ESImageTypeOriginal) {
                NSInteger index = [cacheImagesList indexOfObject:one];
                
                one.imageData = image.imageData;
                [cacheImagesList replaceObjectAtIndex:index withObject:one];
            } else {
                
                ImageOperation *operation = [ImageResizeOperation ResizeOperationManager:self withAction:@selector(didEndOperationWithResult:withContext:) withTarget:@"resize" withUrl:one.imageUrl withImageOprationType:one.imageType withOriginalImage:image.image withSize:one.size withResizeQulity:one.qulity isImageCache:YES withContext:nil];

                [self.operationQueue addOperation:operation];
            }
        }
    }
}

-(void)deleteCacheForImageUrl:(NSString *)imageUrl {
    NSMutableArray *deleteList = [NSMutableArray array];
    for (int i = 0; i < [cacheImagesList count]; i++) {
        CacheImage *one = [cacheImagesList objectAtIndex:i];
        if ([one.imageUrl isEqualToString:imageUrl]) {
            [deleteList addObject:one];
        }
    }
    [cacheImagesList removeObjectsInArray:deleteList];
}
@end
