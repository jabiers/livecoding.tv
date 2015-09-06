//
//  ImageManager.h
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 29..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "ESImage-Constants.h"

/** ESImage와 ESImageView를 제어하기 위한 싱글턴 객체이다.
 
 
 매크로 상수
 MAX_CONCURRENT_OPERATION_COOUNT 최대로 사용할 오퍼레이션의 갯수
 MAX_CACHEIMAGE_COUNT 최대 이미지 캐시에 사용될 공간
 DELETED_IMAGE_COUNT_ONETIME 최대 이미지 캐시의 갯수가 넘어갈 경우 지워질 캐시의 갯수
 
 프러퍼티
 operationQueue 이미지 처리에 사용되는 오퍼레이션 큐이다.
 
 팩토리 메쏘드
 shareInstance 싱글턴 객체를 반환받기 위한 팩토리 메쏘드이다.
 
 일반 메쏘드
 removeAllOperation 현재 큐에 담겨있는 모든 오퍼레이션을 취소시킨다.
 getImageUrl:withTarget: 비동기식으로 이미지를 처리하기 위한 메쏘드로써 imageUrl 과 해당 이미지 뷰를 파라메터값으로 받는다.
 getImageUrl:withTarget:withType: 비동기식으로 이미지를 처리하기 위한 메쏘드로써 imageUrl과 해당 이미지뷰와 타잎을 파라메터값으로 받는다.
 
 
 */
//author Dae-hyun Kim
@class ImageManager;
@class CacheImage;
@class ESImage;

@protocol ImageManagerDelegate <NSObject>

-(void)imageManager:(ImageManager *)imageManager didFailLoadImageUrl:(NSString*)imageUrl withError:(NSError*)error withContext:(id)context;
-(void)imageManager:(ImageManager *)imageManager DidLoadImage:(CacheImage*)image withContext:(id)context;

@end
@interface CacheImage : NSObject

@property (nonatomic, strong) NSString *imageUrl;
@property (nonatomic, assign) ESImageResizeQulity qulity;
@property (nonatomic, assign) CGSize size;
@property (nonatomic, assign) ESImageType imageType;
@property (nonatomic, assign) NSInteger refCount;
@property (nonatomic, strong) NSData *imageData;
@property (nonatomic, strong) NSDate *timeStamp;
@end

@interface ImageManager : NSObject {
    NSMutableDictionary *cacheImages;
    NSMutableArray *cacheImagesIndex;
    NSMutableArray *cacheImagesList;
}
@property     NSMutableArray *cacheImagesList;
@property BOOL test;

@property (nonatomic, strong) NSOperationQueue *operationQueue;

+(ImageManager*)shareInstance;

-(void)removeAllOperation;
-(void)removeCacheImages;

-(void)addCacheImageWithImageUrl:(NSString*)imageUrl withContext:(id)context;

-(BOOL)getImageUrl:(NSString*)imageUrl
            withTarget:(id)target
           withContext:(id)context;

-(BOOL)getImageUrl:(NSString*)imageUrl
            withTarget:(id)target
              withType:(ESImageType)imageType
           withContext:(id)context;

-(BOOL)getImageUrl:(NSString*)imageUrl
            withTarget:(id)target
              withSize:(CGSize)size
           withContext:(id)context;

-(BOOL)getImageUrl:(NSString*)imageUrl
            withTarget:(id)target
              withType:(ESImageType)imageType
          isImageCache:(BOOL)isImageCache
           withContext:(id)context;

-(BOOL)getImageUrl:(NSString*)imageUrl
            withTarget:(id)target
              withSize:(CGSize)size
          isImageCache:(BOOL)isImageCache
           withContext:(id)context;

-(BOOL)getImageUrl:(NSString*)imageUrl
            withTarget:(id)target
              withType:(ESImageType)imageType
              withSize:(CGSize)size
          isImageCache:(BOOL)isImageCache
           withContext:(id)context;

-(BOOL)getImageUrl:(NSString*)imageUrl
            withTarget:(id)target
              withType:(ESImageType)imageType
              withSize:(CGSize)size
      withResizeQulity:(ESImageResizeQulity)resizeQulity
          isImageCache:(BOOL)isImageCache
           withContext:(id)context;


-(void)asynchronousImageLoadWithImageUrl:(NSString *)imageUrl
                               withBlock:(void(^)(NSData *image, NSError *error, BOOL success))block;

-(CacheImage*)synchronousImageLoadWithImageUrl:(NSString *)imageUrl
                                      withSize:(CGSize)size
                                      withType:(ESImageType)imageType
                              withResizeQulity:(ESImageResizeQulity)resizeQulity
                                  isImageCache:(BOOL)isImageCache;

//-(UIImage*)resizingImage:(UIImage*)image
//                  toSize:(CGSize)size
//       withInterpolation:(ESImageResizeQulity)resizeQulity;
//
-(void)replaceCacheImage:(ESImage *)image forImageUrl:(NSString *)imageUrl;
-(void)deleteCacheForImageUrl:(NSString *)imageUrl;
//-(NSData *)checkAndMakePngFromPDF:(NSData *)data;
@end
