//
//  ESImage.h
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 29..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ESImage-Constants.h"
#import "ImageManager.h"

/** Custom UIImage 로써 ImageManager에서 제어한다.
Properties
    imageUrl
    imageType
 
Factory Methods
    imageForUrl: 레퍼런스에 해당하는 이미지를 리턴한다.
    imageForUrl:withImageType: 래펀런스에 해당하는 이미지를 리턴한다.
 
Instance Methods
    setImageUrl: 해당하는 레퍼런스 값으로 세팅한다.
    setImageType: 해당하는 타입으로 세팅한다.
    setImageUrlwithImageType: 해당하는 레퍼런스와 타잎으로 세팅한다.
    모두 비동기 식으로 해당하는 래퍼런스를 받아와 저장한다.
 
현재 ESImage 는 구현된 인터페이스만 작동한다.
 
 */
//author Dae-hyun Kim



typedef enum {
    ESImageFormatUnknown,
    ESImageFormatJPEG,
    ESImageFormatPNG,
    ESImageFormatGIF,
    ESImageFormatTIFF,
    ESImageFormatPDF
} ESImageFormat;


@interface ESImage : UIImage <ImageManagerDelegate> {
    NSDictionary *dic; // 현재 미구현 ESImage 하나로 타입별 이미지를 저장하기 위한 공간
}
@property (nonatomic, assign) ESImageFormat imageFormat;
@property (nonatomic, strong) UIImage *image;

@property (nonatomic, copy) NSString *imageUrl;
@property (nonatomic, assign) ESImageType imageType;
@property (nonatomic, assign, setter = isImageCache:) BOOL isImageCache;

-(id)initWithUIImage:(UIImage *)image;

+(ESImage *)imageForImageUrl:(NSString *)imageUrl withContext:(id)context;
+(ESImage *)imageForImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType withContext:(id)context;
+(ESImage *)imageForImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType isImageCache:(BOOL)isImageCache withContext:(id)context;

-(void)setImageUrl:(NSString *)imageUrl withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl isImageCache:(BOOL)isImageCache withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType isImageCache:(BOOL)isImageCache withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withSize:(CGSize)size withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withSize:(CGSize)size isImageCache:(BOOL)isImageCache withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withResizeQulity:(ESImageResizeQulity)resizeQulity withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withResizeQulity:(ESImageResizeQulity)resizeQulity isImageCache:(BOOL)isImageCache withContext:(id)context;

-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType withSize:(CGSize)size withResizeQulity:(ESImageResizeQulity)resizeQulity isImageCache:(BOOL)isImageCache withContext:(id)context;


+(ESImage*)imageWithData:(NSData*)imageData;
+(ESImageFormat)imageFormatForImageData:(NSData *)data;

-(void)isImageCache:(BOOL)isImageCache;
-(NSData*)imageData;

@end
