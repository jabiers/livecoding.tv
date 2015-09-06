//
//  ESGridView.h
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 27..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import <UIKit/UIKit.h>
//#import "ESGridViewLayoutStrategies.h"
#import "ESGridViewLayoutStrategies.h"
#import "ESGridView-Constants.h"
#import "ESGridViewCell.h"


struct ESGridMatrix {
    int row, col;
};

@class ESGridView;
@class ESGridViewCell;
/** GridView의 내용을 처리해줄 프로토콜
 
 프로토콜 메쏘드
 gridView:shouldSelectAtIndex: 리턴결과를 통해 선택 가능여부 설정
 gridView:willSelectAtIndex: 셀 선택전 불려지는 메쏘드
 gridView:didSelectedAtIndex: 셀 선택후 불려지는 메쏘드
 gridView:didTouchInEmptySpace: 빈공간 선택시 불려지는 메쏘드
 
 gridView:willDeselectedAtIndex: 선택해제되기 전 불려지는 메쏘드(현재 미구현)
 gridView:didDeselectedAtIndex:  선택해제된 후 불려지는 메쏘드(현재 미구현)
 
 gridView:willChangeWithPreGridSize: 그리드사이즈가 변경되기 전에 불려지는 메쏘드
 gridView:didChangedWithApferGridSize: 그리드사이즈가 변경된 후 불려지는 메쏘드
 
 gridView:willChangePage: 페이지가 변경되기 전 불려지는 메쏘드
 gridView:didChangedPage: 페이지가 변경된 후 불려지는 메쏘드
 
 */
//author Dae-hyun Kim
@protocol ESGridViewDelegate <UIScrollViewDelegate, NSObject>

@optional
-(BOOL)gridView:(ESGridView*)gridView shouldSelectAtIndex:(NSInteger)index;
-(void)gridView:(ESGridView*)gridView willSelectAtIndex:(NSInteger)index;
-(void)gridView:(ESGridView*)gridView didSelectedAtIndex:(NSInteger)index;

-(void)gridView:(ESGridView*)gridView didTouchInEmptySpace:(NSNull*)nullValue;

-(void)gridView:(ESGridView*)gridView willDeSelectAtIndex:(NSInteger)index;
-(void)gridView:(ESGridView*)gridView didDeselectedAtIndex:(NSInteger)index;

-(void)gridView:(ESGridView*)gridView willChangeWithPreGridSize:(CGSize)size;
-(void)gridView:(ESGridView*)gridView didChangeWithAfterGridSize:(CGSize)size;

-(void)gridView:(ESGridView*)gridView willChangePage:(NSInteger)page;
-(void)gridView:(ESGridView *)gridView didChangedPage:(NSInteger)page;

-(void)gridView:(ESGridView *)gridView willContentOffsetChange:(CGPoint)contentOffset;
-(void)gridView:(ESGridView *)gridView didContentOffsetChange:(CGPoint)contentOffset;

-(void)gridView:(ESGridView *)gridView changedEdit:(BOOL)editing;
-(void)gridView:(ESGridView *)gridView processDeleteActionForItemAtIndex:(NSInteger)index;
@end
/** GridView 의 데이터를 처리해줄 프로토콜
 
 프로토콜 메쏘드
 numberOfTotalGrid: 그리드에 사용될 데이터들의 갯수를 리턴
 gridView:cellForIndex: 해당 인덱스에 해당하는 셀을 리턴
 
 */
//author Dae-hyun Kim
@protocol ESGridViewDataSource <NSObject>
@optional
-(BOOL)gridView:(ESGridView *)gridView canDeleteItemAtIndex:(NSInteger)index;

@required
-(NSInteger)numberOfTotalGrid:(ESGridView*)gridView;
-(ESGridViewCell*)gridView:(ESGridView*)gridView cellForIndex:(NSInteger)index;

@end

/** UIScrollView 를 확장한 그리드뷰이다.
 
 매크로상수
 kTagOffset 해당 인덱스와 태그값의 차이
 kDefaultSize 기본적용되는 그리드 사이즈 180, 130
 
 프러퍼티
 dataSource GridView의 dataSoure
 delegate GridView의 delegate
 defaultGridSize 기본 그리드 사이즈 값으로 확대 축소시 기준이 되는 그리드 사이wm 기본값은 180, 130
 currentItemSize 현재 Grid의 사이즈
 
 allowGridScaling 그리드의 줌 기능 활성 여부, 기본값 NO
 minimumGridZoomScale 그리드 최대 줌 스케일, 기본값 1.0f
 maximumGridZoomScale 그리드 최소 줌 스케일, 기본값 1.0f
 margin 그리드간 간격
 minEdgeInset 그리드뷰 안에 들어갈 그리드와의 최소 간격
 currentPage 현재 그리드 페이지
 totalPage 전체 그리드 페이지
 
 
 일반 메쏘드
 setAllowGridScaling: 그리드 줌 기능을 활성화 할지 여부를 세팅
 setDataSource: 그리드의 데이터소스를 세팅
 setCurrentGridSize: 현재 그리드 사이즈를 변경
 setCurrentPage: 현재 그리드 페이지를 변경
 setCurrentPage:withAnimation: 현재 그리드 페이지 변경과 동시에 에니메이션 효과 설정
 setLayoutStrategyType 현재 레이아웃 타입 설정
 
 dequeueResuableCell 재활용 가능한 셀 리턴
 dequeueReusableCellWithIdentifier: 해당 아이덴티에 맞는 재활용 가능한 셀을 리턴
 
 cellForGridAtIndex: 해당 인덱스의 셀을 리턴
 
 reloadData 그리드를 새로 그림
 
 */
//author Dae-hyun Kim

typedef enum {
    ESGridViewLayoutType_Horizonal,
    ESGridViewLayoutType_Vertical
} ESGridViewLayoutType;

@interface ESGridView : UIScrollView

@property (nonatomic, es_weak) id <ESGridViewDataSource> dataSource;
@property (nonatomic, es_weak) id <ESGridViewDelegate> delegate;
@property (nonatomic) CGSize defaultGridSize;
@property (nonatomic) CGSize currentGridSize;

@property (nonatomic, assign) BOOL allowGridScaling;
@property(nonatomic) float minimumGridZoomScale;     // default is 1.0
@property(nonatomic) float maximumGridZoomScale;     // default is 1.0. must be > minimum zoom scale to enable zooming

@property (nonatomic) NSInteger margin;
@property (nonatomic) UIEdgeInsets minEdgeInset;

@property (nonatomic, assign) NSInteger currentPage;
@property (nonatomic, readonly) NSInteger totalPage;
@property (nonatomic, readonly) NSInteger numberOfItemsPerPage;
@property (nonatomic, strong) id<ESGridViewLayoutStrategy> layoutStrategy;
@property (nonatomic, readonly) ESGridViewLayoutType gridViewLayoutType;

@property (nonatomic, getter=isEditing) BOOL editing;

-(void)setAllowGridScaling:(BOOL)allowGridScaling;
-(void)setCurrentGridSize:(CGSize)currentItemSize;
-(void)setCurrentPage:(NSInteger)currentPage;
-(void)setCurrentPage:(NSInteger)currentPage withAnimation:(BOOL)animation;
-(void)setLayoutStrategyType:(ESGridViewLayoutStrategyType)type;

- (ESGridViewCell *)dequeueReusableCell;
- (ESGridViewCell *)dequeueReusableCellWithIdentifier:(NSString *)identifier;

- (ESGridViewCell *)cellForGridAtIndex:(NSInteger)index;

- (void)reloadData;
-(void)reloadDataAtIndex:(NSInteger)index;
@end
