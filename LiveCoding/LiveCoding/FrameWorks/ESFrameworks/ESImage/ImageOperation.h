//
//  ImageLoadOperation.h
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 29..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "ESImage-Constants.h"

/** 이미지 처리를 위한 오퍼레이션으로 Load 오퍼레이션과 Resize 오퍼레이션의 부모 클래스 역할을 한다.
 
 프러퍼티
 manager 큐를 관리하는 ImageManager 값
 target 해당하는 ESImage 또는 ESImageView
 action 작업을 처리후 불려지게될 seletor 들을 위한 프러퍼티
 imageUrl 작업을 하게될 Image 의 주소값
 imageType 작업을 하게될 Image 의 ESImageType 값
 
 팩토리 메쏘드
 operationManager:withAction:withTarget:withUrl:withImageOperationType: ImageManager, SEL, ESImage Or ESImageView, imageUrl, imageType 값을 프로퍼티로 받게되며 해당하는 operation 을 반환한다.
 
 일반 메쏘드
 initWithManager:withAction:withTarget:withUrl:withImageoperaionType: ImageManager, SEL, ESImage Or ESImageView, imageUrl, imageType 값을 프로퍼티로 받게되며 해당하는 operation 을 반환한다.
 
 
 */
//author Dae-hyun Kim

@interface ImageOperation : NSOperation  { // base Operation
}
@property (nonatomic,   weak) id manager;
@property (nonatomic,   weak) id target;
@property (nonatomic, assign) SEL action;
@property (nonatomic, strong) NSString *imageUrl;
@property (nonatomic, assign) ESImageType imageType;
@property (nonatomic, assign) CGSize size;
@property (nonatomic, assign) BOOL isImageCache;
@property (nonatomic, strong) NSDate *createdTime;
@property (nonatomic, strong) id context;

+ (id)operationManager:(id)theManager
            withAction:(SEL)theAction
            withTarget:(id)theTarget
               withUrl:(NSString*)theImageUrl
 withImageOprationType:(ESImageType)imageType
              withSize:(CGSize)size
          isImageCache:(BOOL)isImageCache
           withContext:(id)context;

- (id)initWithManager:(id)theManager
           withAction:(SEL)theAction
           withTarget:(id)theTarget
              withUrl:(NSString*)theImageUrl
withImageOprationType:(ESImageType)imageType
             withSize:(CGSize)size
         isImageCache:(BOOL)isImageCache
          withContext:(id)context;

-(UIImage*)resizingImage:(UIImage*)originalImage
                withSize:(CGSize)size
        withResizeQulity:(ESImageResizeQulity)resizeQulity;

@end


/** 비동기식으로 Url 정보를 통해 이미지를 다운바기 위한 클래스이다.
 
 
 */
//author Dae-hyun Kim
@interface ImageLoadOperation : ImageOperation

@end

/** 이미지를 리사이징 하기 위한 오퍼레이션이다.
 
 프러퍼티
 manager 큐를 관리하는 ImageManager 값
 target 해당하는 ESImage 또는 ESImageView
 action 작업을 처리후 불려지게될 seletor 들을 위한 프러퍼티
 imageUrl 작업을 하게될 Image 의 주소값
 imageType 작업을 하게될 Image 의 ESImageType 값
 
 팩토리 메쏘드
 ResizeOperationManager:withAction:withTarget:withUrl:withImageOperationType:withOriginalImage: ImageManager, SEL, ESImage Or ESImageView, imageUrl, imageType OrignalImage 값을 프로퍼티로 받게되며 해당하는 operation 을 반환한다.
 
 
 */
//author Dae-hyun Kim

@interface ImageResizeOperation : ImageOperation

@property (nonatomic, strong) UIImage *originalImage;
@property (nonatomic, assign) ESImageResizeQulity resizeQulity;
@property (nonatomic, assign) CGSize size;

+ (id)ResizeOperationManager:(id)theManager
                  withAction:(SEL)theAction
                  withTarget:(id)theTarget
                     withUrl:(NSString*)theImageUrl
       withImageOprationType:(ESImageType)imageType
           withOriginalImage:(UIImage*)originalImage
                    withSize:(CGSize)size
            withResizeQulity:(ESImageResizeQulity)resizeQulity
                isImageCache:(BOOL)isImageCache
                 withContext:(id)context;

@end


typedef void (^BlockCode)(UIImage *image, NSError *error, BOOL success);
@interface ImageOperationWithBlock : ImageOperation
//+ (id)operationWithBlockManager:(id)theManager
//                     withAction:(SEL)theAction
//                     withTarget:(id)theTarget
//                        withUrl:(NSString*)theImageUrl
//          withImageOprationType:(ESImageType)imageType
//              withOriginalImage:(UIImage*)originalImage
//                       withSize:(CGSize)size
//               withResizeQulity:(ESImageResizeQulity)resizeQulity
//                   isImageCache:(BOOL)isImageCache
//                    withContext:(id)context;

-(void)blockTest:(void(^)(UIImage *image, NSError *error, BOOL success)) block;
@property (copy, nonatomic) void (^block) (UIImage *image, NSError *error, BOOL success);



@end