//
//  ImageLoadOperation.m
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 29..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import "ImageOperation.h"
#import "ESImageView.h"

@implementation ImageOperation

+ (id)operationManager:(id)theManager
            withAction:(SEL)theAction
            withTarget:(id)theTarget
               withUrl:(NSString*)theImageUrl
 withImageOprationType:(ESImageType)imageType
              withSize:(CGSize)size
          isImageCache:(BOOL)isImageCache
           withContext:(id)context{
    
    ImageOperation *operation;
    switch (imageType) {
        case ESImageTypeOriginal:
            operation = [[ImageLoadOperation alloc] initWithManager:theManager
                                                         withAction:theAction
                                                         withTarget:theTarget
                                                            withUrl:theImageUrl
                                              withImageOprationType:imageType
                                                           withSize:size
                                                       isImageCache:isImageCache
                                                        withContext:context];
            break;
            
        case ESImageTypeSpecificalSize:
        case ESImageTypeResizeThumbNail:
        case ESImageTypeResize50Percent:
        case ESImageTypeResize25Percent:
            operation = [[ImageResizeOperation alloc] initWithManager:theManager
                                                           withAction:theAction
                                                           withTarget:theTarget
                                                              withUrl:theImageUrl
                                                withImageOprationType:imageType
                                                             withSize:size
                                                         isImageCache:isImageCache
                                                          withContext:context];
            break;
        default:
            break;
    }
    
    return operation;
}

- (id)initWithManager:(id)theManager
           withAction:(SEL)theAction
           withTarget:(id)theTarget
              withUrl:(NSString*)theImageUrl
withImageOprationType:(ESImageType)imageType
             withSize:(CGSize)size
         isImageCache:(BOOL)isImageCache
          withContext:(id)context {
    
    if (self = [super init]) {
        self.manager = theManager;
        self.action = theAction;
        self.target = theTarget;
        self.imageUrl = theImageUrl;
        self.imageType = imageType;
        self.size = size;
        self.isImageCache = isImageCache;
        self.createdTime = [NSDate date];
        self.context = context;
        
        
    }
    return self;
}



- (void)cancel {
    
    [super cancel];
}

- (void)main {
    
    if (!self.target || [self isCancelled]) {
        [self cancel];
        return;
    }
    
    if (NSOrderedAscending == [self.createdTime compare:[[NSUserDefaults standardUserDefaults] valueForKey:@"OperationCancelTime"]]) {
        [self cancel];
    }
    
}

//-(UIImage*)getImageFromImageUrl:(NSString *)imageUrl error:(NSError **)error {
//    NSString *urlString = (__bridge_transfer NSString *) CFURLCreateStringByAddingPercentEscapes(NULL,
//                                                                                           (__bridge CFStringUrl)imageUrl,
//                                                                                           NULL,
//                                                                                           (__bridge CFStringUrl)@"\";@&+,#[]{} ",
//                                                                                           kCFStringEncodingUTF8);
//
//    NSData *imageData;
//    NSURL *url = [NSURL URLWithString:urlString];
//    if (imageUrl) {
//        imageData = [NSData dataWithContentsOfURL:url options:NSDataReadingUncached error:error];
//    }
//    return [UIImage imageWithData:imageData];
//}

-(NSData*)getImageFromImageUrl:(NSString *)imageUrl error:(NSError **)error {
    
    //    NSLog(imageUrl, @"Image Get URL");
    
    NSURL *url = nil;
    
    
    static NSString *http = @"http://";
    static NSString *https = @"https://";
    
    BOOL isHttpUrl = NO;
    if([imageUrl compare:http options:NSCaseInsensitiveSearch range:NSMakeRange(0, http.length)] == NSOrderedSame ||
       [imageUrl compare:https options:NSCaseInsensitiveSearch range:NSMakeRange(0, https.length)] == NSOrderedSame) {
        isHttpUrl = YES;
    }
    
    if(isHttpUrl == YES)
        url = [NSURL URLWithString:imageUrl];
    else
        url = [NSURL fileURLWithPath:imageUrl isDirectory:NO];
    
    NSData *imageData;
    if (imageUrl) {
        imageData = [NSData dataWithContentsOfURL:url options:NSDataReadingMappedAlways error:error];
#ifdef APP_OPTION_AutoConversionPDFInManager
        imageData = [[ImageManager shareInstance] checkAndMakePngFromPDF:imageData];
#endif
    }
    return imageData;
}

-(UIImage*)resizingImage:(UIImage*)originalImage withSize:(CGSize)size withResizeQulity:(ESImageResizeQulity)resizeQulity {
    if(originalImage == nil) return nil;
    
    float scale = [[UIScreen mainScreen] respondsToSelector:@selector(scale)]?[[UIScreen mainScreen] scale]:1.0f;
    
    CGRect newRect = CGRectIntegral(CGRectMake(0, 0, size.width * scale, size.height * scale));
    
    CGImageRef imageUrl = CGImageCreateCopy(originalImage.CGImage);
    
    // Build a context that's the same dimensions as the new size
    CGContextRef bitmap = CGBitmapContextCreate(NULL,
                                                newRect.size.width,
                                                newRect.size.height,
                                                CGImageGetBitsPerComponent(imageUrl),
                                                0,
                                                CGImageGetColorSpace(imageUrl),
                                                CGImageGetBitmapInfo(imageUrl));
    CGContextConcatCTM(bitmap, CGAffineTransformIdentity);
    
    // Set the quality level to use when rescaling
    CGContextSetInterpolationQuality(bitmap, resizeQulity);
    
    // Draw into the context; this scales the image
    CGContextDrawImage(bitmap, newRect, imageUrl);
    
    // Get the resized image from the context and a UIImage
    CGImageRef newImageUrl = CGBitmapContextCreateImage(bitmap);
    
    //    UIImage *newImage = [UIImage imageWithData:data];
    UIImage *newImage = [UIImage imageWithCGImage:newImageUrl scale:1.0f orientation:UIImageOrientationUp];
    
    // Clean up
    CGImageRelease(imageUrl);
    CGContextRelease(bitmap);
    CGImageRelease(newImageUrl);
    
    return newImage;
}


@end

@implementation ImageLoadOperation

- (void)main {
    
    [super main];
    if (!self.target || [self isCancelled]) {
        [self cancel];
        return;
    }
    
    if ([self.target isKindOfClass:[ESImageView class]]) {
        ESImageView *temp = self.target;
        if (temp.imageUrl != self.imageUrl) {
            return;
        }
    }
    
    NSError *error;
    
    NSData *imageData = [self getImageFromImageUrl:self.imageUrl error:&error];
    UIImage *image = [UIImage imageWithData:imageData];
    
    if (self.target) {
        NSDictionary *result =  @{@"!target":self.target,
                                  @"!imageUrl":self.imageUrl,
                                  @"!imageType":[NSString stringWithFormat:@"%d",self.imageType],
                                  @"!imageSize":[NSValue valueWithCGSize:image.size],
                                  @"!isImageCache":[NSNumber numberWithBool:self.isImageCache],
                                  @"!error":error?error:[NSNull null],
                                  @"!imageData":imageData?imageData:[NSData data],
                                  @"!context":self.context?self.context:@""};
        if ([self.manager respondsToSelector:self.action]) {
//        [self.manager performSelectorOnMainThread:self.action withObject:result waitUntilDone:NO];
            dispatch_async(dispatch_get_main_queue(), ^{
                [self.manager performSelector:self.action withObject:result withObject:self.context];
            });
        }
    }
}

@end

@implementation ImageResizeOperation

+(id)ResizeOperationManager:(id)theManager
                 withAction:(SEL)theAction
                 withTarget:(id)theTarget
                    withUrl:(NSString *)theImageUrl
      withImageOprationType:(ESImageType)imageType
          withOriginalImage:(UIImage *)originalImage
                   withSize:(CGSize)size
           withResizeQulity:(ESImageResizeQulity)resizeQulity
               isImageCache:(BOOL)isImageCache
                withContext:(id)context {
    
    ImageResizeOperation *operation = [[ImageResizeOperation alloc] initWithManager:theManager withAction:theAction withTarget:theTarget withUrl:theImageUrl withImageOprationType:imageType withSize:size isImageCache:isImageCache withContext:context];
    operation.originalImage = originalImage;
    operation.resizeQulity = resizeQulity;
    operation.size = size;
    return operation;
    
}

- (void)main {
    [super main];
    
    if (!self.target || [self isCancelled]) {
        [self cancel];
        return;
    }
    
    if ([self.target isKindOfClass:[ESImageView class]]) {
        ESImageView *temp = self.target;
        if (temp.imageUrl != self.imageUrl) {
            return;
        }
    }
    
    NSError *error;
    UIImage *resizedImage;
    NSData *imageData;
    if (!self.originalImage) {
        
        imageData = [self getImageFromImageUrl:self.imageUrl error:&error];
        self.originalImage = [UIImage imageWithData:imageData];
        
        NSDictionary *result =  @{@"!target":[NSNull null],
                                  @"!imageUrl":self.imageUrl?self.imageUrl:@"",
                                  @"!imageType":[NSString stringWithFormat:@"%d",ESImageTypeOriginal],
                                  @"!imageSize":[NSValue valueWithCGSize:self.originalImage?self.originalImage.size:self.size],
                                  @"!isImageCache":[NSNumber numberWithBool:YES],
                                  @"!error":error?error:[NSNull null],
                                  @"!imageData":imageData?imageData:[NSData data],
                                  @"!context":self.context?self.context:@""};
        if ([self.manager respondsToSelector:self.action])
            //            [self.manager performSelectorOnMainThread:self.action withObject:result waitUntilDone:NO];
            dispatch_async(dispatch_get_main_queue(), ^{
                [self.manager performSelector:self.action withObject:result withObject:self.context];
            });
    } else {
    }
    resizedImage = [self resizingImage:self.originalImage withSize:self.size withResizeQulity:self.resizeQulity];
    
    NSData *data = UIImagePNGRepresentation(resizedImage);
    
    NSDictionary *result =  @{@"!target":self.target?self.target:[NSNull null],
                              @"!imageUrl":self.imageUrl?self.imageUrl:@"",
                              @"!imageType":[NSString stringWithFormat:@"%d",self.imageType],
                              @"!imageSize":[NSValue valueWithCGSize:resizedImage.size],
                              @"!isImageCache":[NSNumber numberWithBool:self.isImageCache],
                              @"!error":error?error:[NSNull null],
                              @"!imageData":data?data:@"",
                              @"!context":self.context?self.context:@"",
                              @"!resizeQulity":[NSString stringWithFormat:@"%d", self.resizeQulity]};
    
    if ([self.manager respondsToSelector:self.action]) {
        //        [self.manager performSelectorOnMainThread:self.action withObject:result waitUntilDone:NO];
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.manager performSelector:self.action withObject:result withObject:self.context];
        });
    }
    
    
}


@end


@implementation ImageOperationWithBlock

//+ (id)operationWithBlockManager:(id)theManager
//                     withAction:(SEL)theAction
//                     withTarget:(id)theTarget
//                        withUrl:(NSString*)theImageUrl
//          withImageOprationType:(ESImageType)imageType
//              withOriginalImage:(UIImage*)originalImage
//                       withSize:(CGSize)size
//               withResizeQulity:(ESImageResizeQulity)resizeQulity
//                   isImageCache:(BOOL)isImageCache
//                    withContext:(id)context {
//
//    _block;
//}

-(void)blockTest:(void (^)(UIImage *, NSError *, BOOL))block {
    
}

@end