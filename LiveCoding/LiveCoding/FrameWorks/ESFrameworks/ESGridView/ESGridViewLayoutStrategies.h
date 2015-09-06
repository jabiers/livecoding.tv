//
//  ESGridViewLayoutStrategies.h
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 27..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ESGridView-Constants.h"
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>


/** 그리드뷰에서 그리드를 나타내는 방법의 대한 클래스
 총 6가지의 형태로 화면을 구성한다.
 
 ESGridViewLayoutVertical 수직으로 그리드를 구성
 ESGridViewLayoutHorizontal 수평으로 그리드를 구성
 ESGridViewLayoutHorizontalPagedLTR 수평으로 하되 한 페이지에서는 왼쪽부터 오른쪽으로 그리드를 구성
 ESGridViewLayoutHorizontalPagedTTB 수평으로 하되 한 페이지에서는 위쪽부터 아래쪽으로 그리드를 구성
 ESGridViewLayoutVerticalPagedLTR 수직으로 하되 한 페이지에서는 왼쪽부터 오른쪽으로 그리드를 구성 (구현중)
 ESGridViewLayoutVerticalPagedTTB 수직으로 하되 한 페이지에서는 위쪽부터 아래쪽으로 그리드를 구성 (구현중)

 팩토리 메쏘드
 strategyFromType: 해당하는 타잎의 객체를 반환한다.
 */
//author Dae-hyun Kim

@protocol ESGridViewLayoutStrategy;

typedef enum {
    ESGridViewLayoutVertical = 0,
    ESGridViewLayoutHorizontal,
    ESGridViewLayoutHorizontalPagedLTR,   // LTR: left to right
    ESGridViewLayoutHorizontalPagedTTB,    // TTB: top to bottom
    ESGridViewLayoutVerticalPagedLTR,
    ESGridViewLayoutVerticalPagedTTB
} ESGridViewLayoutStrategyType;

//////////////////////////////////////////////////////////////
#pragma mark - Strategy Factory
//////////////////////////////////////////////////////////////

@interface ESGridViewLayoutStrategyFactory : NSObject

+ (id<ESGridViewLayoutStrategy>)strategyFromType:(ESGridViewLayoutStrategyType)type;

@end

@interface ESGridViewLayoutStrategies : NSObject

@end


/** 그리드뷰 레이아웃에 대한 프로토콜
 
프로토콜
    requiresEnablingPaging 페이징이 가능한 레이아웃인지의 여부
    type 현재 레이아웃 타입 리턴
    setupItemSize:andItemSpacing:withMinEdgeInsets:andCenteredGrid 아이템 사이즈와 여백 엣지 센터에 대해 설정
    rebaseWithItemCount:insideOfBounds 아이템 갯수와 바운스 값으로 재 계산
    contentSize 현재 컨텐트 사이즈
    originForItemAtPosition: 현재 그리드의 x,y 좌표를 설정
    itemPositionFromLocation: 해당 위치에 대한 그리드 인덱스 값
    rangeOfPositionsInBoundsFromOffset: 현재 offset 에 대한 range 값
 
 */
//author Dae-hyun Kim
@protocol ESGridViewLayoutStrategy <NSObject>

+ (BOOL)requiresEnablingPaging;

- (ESGridViewLayoutStrategyType)type;

// Setup
- (void)setupItemSize:(CGSize)itemSize andItemSpacing:(NSInteger)spacing withMinEdgeInsets:(UIEdgeInsets)edgeInsets andCenteredGrid:(BOOL)centered;

// Recomputing
- (void)rebaseWithItemCount:(NSInteger)count insideOfBounds:(CGRect)bounds;

// Fetching the results
- (CGSize)contentSize;
- (CGPoint)originForItemAtPosition:(NSInteger)position;
- (NSInteger)itemPositionFromLocation:(CGPoint)location;

- (NSRange)rangeOfPositionsInBoundsFromOffset:(CGPoint)offset;

@end


/** 그리드뷰에서 그리드들을 나타내는 방법의 대한 부모클래스
 
 프러퍼티 
    type 현재 그리드 타입
    itemSize 그리드 사이즈
    itemSpacing 그리드간 여백
    minEdgeInsets 그리드뷰와 그리드간의 최소 여백
    centeredGrid 중심의 위치했는지 여부
    itemCount 그리드 갯수
    edgeInsets 그리드뷰와 그리드간의 여백
    gridBounds 그리드의 바운스
    contentSize 현재 컨탠트 사이즈
 
 일반 메쏘드
    setupItemSize:andItemSpacing:withMinEdgeInsets:andCenteredGrid: 아이템 사이즈와 여백 엣지 센터에 대해 설정
    setEdgeAndContentSizeFromAbsoluteContentSize: 실제 컨탠트 사이즈로부터의 절대적인 여백값
 */
//author Dae-hyun Kim
@interface ESGridViewLayoutStrategyBase : NSObject {
@protected
    // All of these vars should be set in the init method
    ESGridViewLayoutStrategyType _type;
    
    // All of these vars should be set in the setup method of the child class
    CGSize _itemSize;
    NSInteger _itemSpacing;
    UIEdgeInsets _minEdgeInsets;
    BOOL _centeredGrid;
    
    // All of these vars should be set in the rebase method of the child class
    NSInteger _itemCount;
    UIEdgeInsets _edgeInsets;
    CGRect _gridBounds;
    CGSize _contentSize;
}

@property (nonatomic, readonly) ESGridViewLayoutStrategyType type;

@property (nonatomic, readonly) CGSize itemSize;
@property (nonatomic, readonly) NSInteger itemSpacing;
@property (nonatomic, readonly) UIEdgeInsets minEdgeInsets;
@property (nonatomic, readonly) BOOL centeredGrid;

@property (nonatomic, readonly) NSInteger itemCount;
@property (nonatomic, readonly) UIEdgeInsets edgeInsets;
@property (nonatomic, readonly) CGRect gridBounds;
@property (nonatomic, readonly) CGSize contentSize;

// Protocol methods implemented in base class
- (void)setupItemSize:(CGSize)itemSize andItemSpacing:(NSInteger)spacing withMinEdgeInsets:(UIEdgeInsets)edgeInsets andCenteredGrid:(BOOL)centered;

// Helpers
- (void)setEdgeAndContentSizeFromAbsoluteContentSize:(CGSize)actualContentSize;

- (NSInteger)getNumberOfItemsPerPage;
@end


/** 수직으로 표시하는 방법에 대한 클래스
 
 프러퍼티
    _numberOfItemsPerRow 한 줄에 표시할수 있는 그리드의 갯수
 */
//author Dae-hyun Kim

//////////////////////////////////////////////////////////////
#pragma mark - Vertical strategy
//////////////////////////////////////////////////////////////

@interface ESGridViewLayoutVerticalStrategy : ESGridViewLayoutStrategyBase <ESGridViewLayoutStrategy>
{
@protected
    NSInteger _numberOfItemsPerRow;
}

@property (nonatomic, readonly) NSInteger numberOfItemsPerRow;

@end


/** 수평으로 표시하는 방법에 대한 클래스
 
 프러퍼티
    _numberOfItemsPerColumn 한 열에 표시할수 있는 그리드의 갯수
 */
//author Dae-hyun Kim

//////////////////////////////////////////////////////////////
#pragma mark - Horizontal strategy
//////////////////////////////////////////////////////////////

@interface ESGridViewLayoutHorizontalStrategy : ESGridViewLayoutStrategyBase <ESGridViewLayoutStrategy>
{
@protected
    NSInteger _numberOfItemsPerColumn;
}

@property (nonatomic, readonly) NSInteger numberOfItemsPerColumn;

@end


/** 수평으로 표시하되 페이지로 그리드를 나타내는 방법에 대한 클래스
 
 프러퍼티
    numberOfItemsPerRow 한줄에 표시할수 있는 그리드의 갯수
    numberOfItemsPerPage 한페이지에 나타낼수 있는 그리드의 갯수
    numberOfPages 총 페이지 수
 
 일반 메쏘드
 positionForItemAtColumn:row:page: 현재 칼럼과 로우와 페이지에 대해서 그리드의 인덱스 값
 columnForItemAtPosition: 포지션에 대한 칼럼값
 rowForItemAtPosition: 포지션에 대한 로우 값
 */
//author Dae-hyun Kim
//////////////////////////////////////////////////////////////
#pragma mark - Horizontal Paged strategy (LTR behavior)
//////////////////////////////////////////////////////////////

@interface ESGridViewLayoutHorizontalPagedStrategy : ESGridViewLayoutHorizontalStrategy
{
@protected
    NSInteger _numberOfItemsPerRow;
    NSInteger _numberOfItemsPerPage;
    NSInteger _numberOfPages;
}

@property (nonatomic, readonly) NSInteger numberOfItemsPerRow;
@property (nonatomic, readonly) NSInteger numberOfItemsPerPage;
@property (nonatomic, readonly) NSInteger numberOfPages;


// Only these 3 methods have be reimplemented by child classes to change the LTR and TTB kind of behavior
- (NSInteger)positionForItemAtColumn:(NSInteger)column row:(NSInteger)row page:(NSInteger)page;
- (NSInteger)columnForItemAtPosition:(NSInteger)position;
- (NSInteger)rowForItemAtPosition:(NSInteger)position;

@end

/** 수평으로 표시하되 왼쪽에서 오른쪽으로 그리드를 나타내는 방법에 대한 클래스
 
 */
//author Dae-hyun Kim
//////////////////////////////////////////////////////////////
#pragma mark - Horizontal Paged Left to Right strategy
//////////////////////////////////////////////////////////////

@interface ESGridViewLayoutHorizontalPagedLTRStrategy : ESGridViewLayoutHorizontalPagedStrategy

@end

/** 수평으로 표시하되 위쪽에서 아래쪽으로 그리드를 나타내는 방법에 대한 클래스
 
 */
//author Dae-hyun Kim
//////////////////////////////////////////////////////////////
#pragma mark - Horizontal Paged Top To Bottom strategy
//////////////////////////////////////////////////////////////

@interface ESGridViewLayoutHorizontalPagedTTBStrategy : ESGridViewLayoutHorizontalPagedStrategy

@end


@interface ESGridViewlayoutVerticalPagedStrategy : ESGridViewLayoutVerticalStrategy
{
    @protected
    NSInteger _numberOfItemsPerColumn;
    NSInteger _numberOfItemsPerPage;
    NSInteger _numberOfPages;
}

@property (nonatomic, readonly) NSInteger numberOfItemsPerColumn;
@property (nonatomic, readonly) NSInteger numberOfItemsPerPage;
@property (nonatomic, readonly) NSInteger numberOfPages;

- (NSInteger)positionForItemAtColumn:(NSInteger)column row:(NSInteger)row page:(NSInteger)page;
- (NSInteger)columnForItemAtPosition:(NSInteger)position;
- (NSInteger)rowForItemAtPosition:(NSInteger)position;

@end


@interface ESGridViewlayoutVerticalPagedLTRStrategy : ESGridViewlayoutVerticalPagedStrategy

@end

@interface ESGridViewlayoutVerticalPagedTTBStrategy : ESGridViewlayoutVerticalPagedStrategy

@end