//
//  ESImageView.m
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 29..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import "ESImageView.h"
#import "ESImage.h"
#import <QuartzCore/QuartzCore.h>

@implementation ESImageView
@synthesize imageUrl = _imageUrl;

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self initialize];
    }
    return self;
}

#pragma mark - initialize

-(void) initialize {
    _state = ESImageViewState_Init;
    _activeIndicator = YES; //default yes
    [self setActiveIndicator:_activeIndicator];
    [self setUserInteractionEnabled:YES];
    _isImageCache = DEFAULT_IMAGE_CACHE;

}

#pragma mark - layout

-(void)layoutSubviews {
}


#pragma mark - setter / getter methods

-(void)setActiveIndicator:(BOOL)activeIndicator {
    _activeIndicator = activeIndicator;
    if (_activeIndicator && _state != ESImageViewState_ImageLoaded && _state != ESImageViewState_Empty) {
        if (!indicator) {
            indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhite];
            indicator.backgroundColor = [UIColor grayColor];
            indicator.frame = CGRectMake(0, 0, 25, 25);
            indicator.layer.cornerRadius = 3.0;
            [indicator setCenter:CGPointMake(self.frame.size.width/2, self.frame.size.height/2)];
            [indicator setAutoresizingMask:UIViewAutoresizingFlexibleBottomMargin | UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin];

            [self addSubview:indicator];
            [indicator startAnimating];
        }
    } else {
        if (indicator) {
            [indicator stopAnimating];
            [indicator removeFromSuperview];
            indicator = nil;
        }
    }
}

-(void)setImage:(id)image {

    if (!image) {
        [super setImage:nil];
//        _imageUrl = nil;
        _imageType = ESImageTypeOriginal;
        _state = ESImageViewState_Empty;

        [indicator removeFromSuperview];
        indicator = nil;
        return;
    }
    
    if (_delegate)
        if ([_delegate respondsToSelector:@selector(imageView:willRequestImageUrl:withContext:)])
            [_delegate imageView:self willRequestImageUrl:self.imageUrl withContext:_context];

    if ([image isKindOfClass:[ESImage class]]) {
        ESImage *tempObj = image;
        [self setImageUrl:tempObj.imageUrl withImageType:tempObj.imageType isImageCache:tempObj.isImageCache withContext:_context];
    } else if ([image isKindOfClass:[CacheImage class]]){

        CacheImage *tempObj = image;
        NSData *imageData = tempObj.imageData;
        UIImage *tempImage = [UIImage imageWithData:imageData];
        [super setImage:tempImage];

        _state = ESImageViewState_ImageLoaded;
    } else {
        [super setImage:image];
        [self setActiveIndicator:NO];
        _state = ESImageViewState_ImageLoaded;

    }
    
    
}


-(void)setImageUrl:(NSString *)imageUrl {
    [self setImageUrl:imageUrl withContext:nil];
}

-(void)setImageUrl:(NSString *)imageUrl withContext:(id)context {
    
    [self setImageUrl:imageUrl isImageCache:_isImageCache withContext:context];
}

-(void)setImageUrl:(NSString *)imageUrl isImageCache:(BOOL)isImageCache withContext:(id)context{
    [self setImageUrl:imageUrl withImageType:DEFAULT_IMAGETYPE withSize:self.frame.size withResizeQulity:DEFAULT_RESIZE_QULITY isImageCache:isImageCache withContext:context];
}

-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType withContext:(id)context{
    [self setImageUrl:imageUrl withImageType:imageType isImageCache:_isImageCache withContext:context];
}

-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType isImageCache:(BOOL)isImageCache withContext:(id)context{

    [self setImageUrl:imageUrl withImageType:imageType withSize:self.frame.size withResizeQulity:DEFAULT_RESIZE_QULITY isImageCache:isImageCache withContext:context];
    
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
    [self setImageUrl:imageUrl withImageType:DEFAULT_IMAGETYPE withSize:self.frame.size withResizeQulity:DEFAULT_RESIZE_QULITY isImageCache:isImageCache withContext:context];
    
}

-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType withSize:(CGSize)size withResizeQulity:(ESImageResizeQulity)resizeQulity isImageCache:(BOOL)isImageCache withContext:(id)context{
    if (imageUrl == nil || [imageUrl isEqualToString:@""]) {
        return;
    
    }
    [self setImage:nil];
    _state = ESImageViewState_WaitForLoadImage;
    [self setActiveIndicator:_activeIndicator];
    
    _imageType = imageType;
    _context = context;


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
    _imageUrl = imageUrl;

    //------------------------------------------------

    [[ImageManager shareInstance] getImageUrl:imageUrl withTarget:self withType:imageType withSize:size withResizeQulity:resizeQulity isImageCache:isImageCache withContext:context];

}

-(void)setImageWithImageLoadConfig:(ESImageLoadConfig)imageConfig withContext:(id)context {
    [self setImageUrl:imageConfig.imageUrl withImageType:imageConfig.imageType withSize:self.frame.size withResizeQulity:imageConfig.resizeQulity isImageCache:imageConfig.isImageCache withContext:context];
}


-(void)imageManager:(ImageManager *)imageManager didFailLoadImageUrl:(NSString *)imageUrl withError:(NSError *)error withContext:(id)context {
    if (_delegate)
        if ([_delegate respondsToSelector:@selector(imageView:didFailReceiveImage:withContext:)])
            [_delegate imageView:self didFailReceiveImage:error withContext:_context];
    
    _state = ESImageViewState_Empty;
    [self setActiveIndicator:_activeIndicator];

}

-(void)imageManager:(ImageManager *)imageManager DidLoadImage:(CacheImage *)image withContext:(id)context {
    [self setImage:image];
    [self setActiveIndicator:_activeIndicator];
    
    if (_delegate)
        if ([_delegate respondsToSelector:@selector(imageView:didReceiveImage:withContext:)])
            [_delegate imageView:self didReceiveImage:image withContext:_context];
}

@end