//
//  ESImage.m
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 29..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import "ESImage.h"
@interface ESImage (private)


//@property (nonatomic, assign) ESImageFormat imageFormat;
//@property (nonatomic, strong) UIImage *image;
@end


@implementation ESImage
@synthesize imageUrl = __imageUrl;
@synthesize imageType = _imageType;
@synthesize imageFormat = _imageFormat;

-(id)self {
    return (ESImage *)_image;
}

+(ESImage*)imageForImageUrl:(NSString*)imageUrl withContext:(id)context{
    return [self imageForImageUrl:imageUrl withImageType:ESImageTypeOriginal withContext:context];
}

+(ESImage*)imageForImageUrl:(NSString*)imageUrl withImageType:(ESImageType)imageType withContext:(id)context{
    return [self imageForImageUrl:imageUrl withImageType:ESImageTypeOriginal isImageCache:DEFAULT_IMAGE_CACHE withContext:context];
}

+(ESImage *)imageForImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType isImageCache:(BOOL)isImageCache withContext:(id)context{
    ESImage *image = [[ESImage alloc] init];
//    [image setImageUrl:imageUrl withImageType:ESImageTypeOriginal isImageCache:isImageCache withContext:context];
//    [image setImageUrl:imageUrl withImageType:ESImageTypeOriginal isImageCache:isImageCache withContext:context];

    [image setImageUrl:imageUrl withImageType:ESImageTypeOriginal withSize:CGSizeZero withResizeQulity:DEFAULT_RESIZE_QULITY isImageCache:DEFAULT_IMAGE_CACHE withContext:nil];
    return image;
}

-(id)init {
    if (self = [super init]) {
        [self initialize];
    }
    return self;
}

-(id)initWithUIImage:(UIImage *)image {
    if (self = [super init]) {
        [self initialize];
        [self setImage:image];
    }
    return self;
}

-(void)initialize {
    _imageFormat = DEFAULT_IMAGE_FORMAT;

    _isImageCache = DEFAULT_IMAGE_CACHE;
    _imageType = ESImageTypeOriginal; // default original
}

#pragma mark - setter / getter Methods
-(void)setImageUrl:(NSString *)imageUrl withContext:(id)context {
    
    [self setImageUrl:imageUrl isImageCache:_isImageCache withContext:context];
}

-(void)setImageUrl:(NSString *)imageUrl isImageCache:(BOOL)isImageCache withContext:(id)context{
    [self setImageUrl:imageUrl withImageType:DEFAULT_IMAGETYPE withSize:self.size withResizeQulity:DEFAULT_RESIZE_QULITY isImageCache:isImageCache withContext:context];
}

-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType withContext:(id)context{
    [self setImageUrl:imageUrl withImageType:imageType isImageCache:_isImageCache withContext:context];
}

-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType isImageCache:(BOOL)isImageCache withContext:(id)context{
    
    [self setImageUrl:imageUrl withImageType:imageType withSize:self.size withResizeQulity:DEFAULT_RESIZE_QULITY isImageCache:isImageCache withContext:context];
    
}
-(void)setImageUrl:(NSString *)imageUrl withSize:(CGSize)size withContext:(id)context{
    
    [self setImageUrl:imageUrl withSize:size isImageCache:_isImageCache withContext:context];
    
}
-(void)setImageUrl:(NSString *)imageUrl withSize:(CGSize)size isImageCache:(BOOL)isImageCache withContext:(id)context{
    
    [self setImageUrl:imageUrl withImageType:DEFAULT_IMAGETYPE withSize:size withResizeQulity:DEFAULT_RESIZE_QULITY isImageCache:isImageCache withContext:context];
    
}
-(void)setImageUrl:(NSString *)imageUrl withResizeQulity:(ESImageResizeQulity)resizeQulity withContext:(id)context{
    
    [self setImageUrl:imageUrl withResizeQulity:resizeQulity isImageCache:_isImageCache withContext:context];
    
}
-(void)setImageUrl:(NSString *)imageUrl withResizeQulity:(ESImageResizeQulity)resizeQulity isImageCache:(BOOL)isImageCache withContext:(id)context{
    [self setImageUrl:imageUrl withImageType:DEFAULT_IMAGETYPE withSize:self.size withResizeQulity:DEFAULT_RESIZE_QULITY isImageCache:isImageCache withContext:context];
    
}

-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType withSize:(CGSize)size withResizeQulity:(ESImageResizeQulity)resizeQulity isImageCache:(BOOL)isImageCache withContext:(id)context{
    
//    [self setImage:nil];
//    _state = ESImageViewState_WaitForLoadImage;
//    [self setActiveIndicator:_activeIndicator];
//    
//    _imageUrl = imageUrl;
//    _imageType = imageType;
//    _context = context;
    
    
    //----------------------------  depend on Daehyun Kim
    if ([[[NSURL URLWithString:imageUrl] scheme] isEqualToString:@"http"] || [[[NSURL URLWithString:imageUrl] scheme] isEqualToString:@"https"]) {
        NSRange r1 = [imageUrl rangeOfString:@" Rev"];
        if(r1.location != NSNotFound)
        {
            NSRange r2 = [imageUrl rangeOfString:@"." options:0 range:NSMakeRange(r1.location, imageUrl.length - r1.location)];
            if(r2.location != NSNotFound)
            {
                NSMutableString *newUrl = [NSMutableString string];
                [newUrl appendString:[imageUrl substringToIndex:r1.location]];
                [newUrl appendString:[imageUrl substringFromIndex:r2.location]];
                imageUrl = newUrl;
            }
        }
    }
    //------------------------------------------------
    
    [[ImageManager shareInstance] getImageUrl:imageUrl withTarget:self withType:imageType withSize:size withResizeQulity:resizeQulity isImageCache:isImageCache withContext:context];
    
}


+(ESImage*)imageWithData:(NSData *)imageData {
    
    ESImage *returnImage = [[ESImage alloc] init];
    
    returnImage.imageFormat = [ESImage imageFormatForImageData:imageData];
    
    [returnImage setImage:[UIImage imageWithData:imageData]];
    
    return returnImage;
}

+(ESImageFormat)imageFormatForImageData:(NSData *)data {
    uint8_t c;
    [data getBytes:&c length:1];
    ESImageFormat ret = ESImageFormatUnknown;
    switch (c) {
        case 0xFF:
            ret = ESImageFormatJPEG;
            break;
        case 0x89:
            ret = ESImageFormatPNG;
            break;
        case 0x47:
            ret = ESImageFormatGIF;
            break;
        case 0x49:
        case 0x4D:
            ret = ESImageFormatTIFF;
            break;
        case 0x25: { //"%PDF"
            char buffer[4];
            [data getBytes:buffer length:4];
            if(buffer[1] == 0x50 && buffer[2] == 0x44 && buffer[3] == 0x46)
                ret = ESImageFormatPDF;
            break;
        }
    }
    return ret;
}

-(void)isImageCache:(BOOL)isImageCache {
    _isImageCache = isImageCache;
    
}
-(ESImageFormat)imageFormat {
    if (!_imageFormat) {
        _imageFormat = ESImageFormatJPEG;
    }
    return _imageFormat;
}

-(NSData *)imageData {
    NSData *ret;
    switch (self.imageFormat) {
        case ESImageFormatJPEG:
            ret = UIImageJPEGRepresentation(self.image, JPEG_compressionQuality);
            break;
        case ESImageFormatTIFF:
        case ESImageFormatPNG:
            ret = UIImagePNGRepresentation(self.image);
        default:
            break;
    }
    return ret;
}

-(CGSize)size {
    return self.image.size;
}

#pragma mark - ImageManager Delegate


-(void)imageManager:(ImageManager *)imageManager didFailLoadImageUrl:(NSString *)imageUrl withError:(NSError *)error withContext:(id)context{
}

-(void)imageManager:(ImageManager *)imageManager DidLoadImage:(CacheImage *)image withContext:(id)context {
    _image = [UIImage imageWithData:image.imageData];
}

@end