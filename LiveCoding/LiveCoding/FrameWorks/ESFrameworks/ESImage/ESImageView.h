//
//  ESImageView.h
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 29..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ESImage-Constants.h"
#import "ImageManager.h"

/** 커스텀 UIImageView 로써 비동기식 이미지 세팅과 인디케이터 실패 메세지 등을 지원하고 ImageManager에서 제어한다.
 
 프러퍼티
 imageUrl 현재 이미지의 레퍼런스 값을 담는다.
 activeIndicator 인디케이터 사용 여부 변수다. 기본 YES이며 NO 로 설정할경우 인디케이터는 나타나지 않는다.
 state 상태값 Init, Empty, WaitForLoadImage, ImageLoaded 의 값을 갖는다.
 failedMessage (구현중) 실패시에 나타날 메세지를 담은 label 이다.
 imageType 현재 이미지의 type 정보를 담는다.
 
 팩토리 메쏘드
 
 일반 메쏘드
 setActiveIndicator: 활성화를 시키게 되면 이미지 로딩시 인디케이터가 나타난다.
 setImage: UIImage의 메쏘드의 오버라이드된 메쏘드이다. 이미지를 세팅할 수 있다. ESImage 객체를 넘기게 되면 비동기식으로 이미지가 세팅된다.
 setImageUrl: 이미지 레퍼런스 정보만 넘기게 된다. Url 을 파라메터값으로 받게 되면 비동기식으로 이미지가 세팅된다.
 setImageRef:withImageType: 이미지 레퍼런스와 이미지 타잎을 파라메터값으로 넘겨주게 되면 비동기식으로 이미지가 세팅된다.
 
 */
//author Dae-hyun Kim


@class ESImageView;
@class CacheImage;
@protocol ESImageViewDelegate <NSObject>

@optional
-(void)imageView:(ESImageView*)imageView willRequestImageUrl:(NSString*)imageUrl withContext:(id)context;

@required
-(void)imageView:(ESImageView*)imageView didReceiveImage:(CacheImage*)cacheImage withContext:(id)context;
-(void)imageView:(ESImageView*)imageView didFailReceiveImage:(NSError*)error withContext:(id)context;

@end

enum ESImageViewState {
    ESImageViewState_Init,
    ESImageViewState_Empty,
    ESImageViewState_WaitForLoadImage,
    ESImageViewState_ImageLoaded,
};

@class ESImage;

@interface ESImageView : UIImageView <ImageManagerDelegate> {
    UIActivityIndicatorView *indicator;
}

@property (nonatomic, copy, setter = setImageUrl:) NSString *imageUrl;
@property (nonatomic, assign) ESImageType imageType;
@property (nonatomic, assign) BOOL activeIndicator;
@property (readonly) enum ESImageViewState state;
@property (nonatomic, assign, setter = isImageCache:) BOOL isImageCache;
@property (nonatomic, weak) id <ESImageViewDelegate> delegate;
@property (nonatomic, strong) id context;

-(void)setActiveIndicator:(BOOL)activeIndicator;
-(void)setImage:(id)image;
-(void)setImageUrl:(NSString *)imageUrl;
-(void)setImageUrl:(NSString *)imageUrl withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl isImageCache:(BOOL)isImageCache withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType isImageCache:(BOOL)isImageCache withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withSize:(CGSize)size withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withSize:(CGSize)size isImageCache:(BOOL)isImageCache withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withResizeQulity:(ESImageResizeQulity)resizeQulity withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withResizeQulity:(ESImageResizeQulity)resizeQulity isImageCache:(BOOL)isImageCache withContext:(id)context;
-(void)setImageUrl:(NSString *)imageUrl withImageType:(ESImageType)imageType withSize:(CGSize)size withResizeQulity:(ESImageResizeQulity)resizeQulity isImageCache:(BOOL)isImageCache withContext:(id)context;
-(void)setImageWithImageLoadConfig:(ESImageLoadConfig)imageConfig withContext:(id)context;

@end
