//
//  ESGridViewLayoutStrategies.m
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 27..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import "ESGridViewLayoutStrategies.h"

//////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark - Factory implementation
//////////////////////////////////////////////////////////////

@implementation ESGridViewLayoutStrategyFactory

+ (id<ESGridViewLayoutStrategy>)strategyFromType:(ESGridViewLayoutStrategyType)type
{
    id<ESGridViewLayoutStrategy> strategy = nil;
    
    switch (type) {
        case ESGridViewLayoutVertical:
            strategy = [[ESGridViewLayoutVerticalStrategy alloc] init];
            break;
        case ESGridViewLayoutHorizontal:
            strategy = [[ESGridViewLayoutHorizontalStrategy alloc] init];
            break;
        case ESGridViewLayoutHorizontalPagedLTR:
            strategy = [[ESGridViewLayoutHorizontalPagedLTRStrategy alloc] init];
            break;
        case ESGridViewLayoutHorizontalPagedTTB:
            strategy = [[ESGridViewLayoutHorizontalPagedTTBStrategy alloc] init];
            break;
        case ESGridViewLayoutVerticalPagedLTR:
            strategy = [[ESGridViewlayoutVerticalPagedLTRStrategy alloc] init];
            break;
        case ESGridViewLayoutVerticalPagedTTB:
            strategy = [[ESGridViewlayoutVerticalPagedTTBStrategy alloc] init];
            break;
    }
    
    return strategy;
}

@end



//////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark - Strategy base class implementation
//////////////////////////////////////////////////////////////

@implementation ESGridViewLayoutStrategyBase

@synthesize type          = _type;

@synthesize itemSize      = _itemSize;
@synthesize itemSpacing   = _itemSpacing;
@synthesize minEdgeInsets = _minEdgeInsets;
@synthesize centeredGrid  = _centeredGrid;

@synthesize itemCount     = _itemCount;
@synthesize edgeInsets    = _edgeInsets;
@synthesize gridBounds    = _gridBounds;
@synthesize contentSize   = _contentSize;


- (void)setupItemSize:(CGSize)itemSize andItemSpacing:(NSInteger)spacing withMinEdgeInsets:(UIEdgeInsets)edgeInsets andCenteredGrid:(BOOL)centered
{
    _itemSize      = itemSize;
    _itemSpacing   = spacing;
    _minEdgeInsets = edgeInsets;
    _centeredGrid  = centered;
}

- (void)setEdgeAndContentSizeFromAbsoluteContentSize:(CGSize)actualContentSize
{
    if (self.centeredGrid)
    {
        NSInteger widthSpace, heightSpace;
        NSInteger top, left, bottom, right;
        
        widthSpace  = floor((self.gridBounds.size.width  - actualContentSize.width)  / 2.0);
        heightSpace = floor((self.gridBounds.size.height - actualContentSize.height) / 2.0);
        
        left   = MAX(widthSpace,  self.minEdgeInsets.left);
        right  = MAX(widthSpace,  self.minEdgeInsets.right);
        top    = MAX(heightSpace, self.minEdgeInsets.top);
        bottom = MAX(heightSpace, self.minEdgeInsets.bottom);
        
        _edgeInsets = UIEdgeInsetsMake(top, left, bottom, right);
    }
    else
    {
        _edgeInsets = self.minEdgeInsets;
    }
    
    _contentSize = CGSizeMake(actualContentSize.width  + self.edgeInsets.left + self.edgeInsets.right,
                              actualContentSize.height + self.edgeInsets.top  + self.edgeInsets.bottom);
}

-(NSInteger)getNumberOfItemsPerPage {
    return [[self performSelector:@selector(numberOfItemsPerPage)] intValue];
}
@end



//////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark - Vertical strategy implementation
//////////////////////////////////////////////////////////////

@implementation ESGridViewLayoutVerticalStrategy

@synthesize numberOfItemsPerRow = _numberOfItemsPerRow;

+ (BOOL)requiresEnablingPaging
{
    return NO;
}

- (id)init
{
    if ((self = [super init]))
    {
        _type = ESGridViewLayoutVertical;
    }
    
    return self;
}

- (void)rebaseWithItemCount:(NSInteger)count insideOfBounds:(CGRect)bounds
{
    _itemCount  = count;
    _gridBounds = bounds;
    
    CGRect actualBounds = CGRectMake(0,
                                     0,
                                     bounds.size.width  - self.minEdgeInsets.right - self.minEdgeInsets.left,
                                     bounds.size.height - self.minEdgeInsets.top   - self.minEdgeInsets.bottom);
    
    _numberOfItemsPerRow = 1;
    
    while ((self.numberOfItemsPerRow + 1) * (self.itemSize.width + self.itemSpacing) - self.itemSpacing <= actualBounds.size.width)
    {
        _numberOfItemsPerRow++;
    }
    
    NSInteger numberOfRows = ceil(self.itemCount / (1.0 * self.numberOfItemsPerRow));
    CGSize actualContentSize = CGSizeMake(ceil(MIN(self.itemCount, self.numberOfItemsPerRow) * (self.itemSize.width + self.itemSpacing)) - self.itemSpacing,
                                          ceil(numberOfRows * (self.itemSize.height + self.itemSpacing)) - self.itemSpacing);
    
    [self setEdgeAndContentSizeFromAbsoluteContentSize:actualContentSize];
}

- (CGPoint)originForItemAtPosition:(NSInteger)position
{
    CGPoint origin = CGPointZero;
    
    if (self.numberOfItemsPerRow > 0 && position >= 0)
    {
        NSUInteger col = position % self.numberOfItemsPerRow;
        NSUInteger row = position / self.numberOfItemsPerRow;
        
        origin = CGPointMake(col * (self.itemSize.width + self.itemSpacing) + self.edgeInsets.left,
                             row * (self.itemSize.height + self.itemSpacing) + self.edgeInsets.top);
    }
    
    return origin;
}

- (NSInteger)itemPositionFromLocation:(CGPoint)location
{
    CGPoint relativeLocation = CGPointMake(location.x - self.edgeInsets.left,
                                           location.y - self.edgeInsets.top);
    
    int col = (int) (relativeLocation.x / (self.itemSize.width + self.itemSpacing));
    int row = (int) (relativeLocation.y / (self.itemSize.height + self.itemSpacing));
    
    int position = col + row * self.numberOfItemsPerRow;
    
    if (position >= [self itemCount] || position < 0)
    {
        position = INVALID_POSITION;
    }
    else
    {
        CGPoint itemOrigin = [self originForItemAtPosition:position];
        CGRect itemFrame = CGRectMake(itemOrigin.x,
                                      itemOrigin.y,
                                      self.itemSize.width,
                                      self.itemSize.height);
        
        if (!CGRectContainsPoint(itemFrame, location))
        {
            position = INVALID_POSITION;
        }
    }
    
    return position;
}

- (NSRange)rangeOfPositionsInBoundsFromOffset:(CGPoint)offset
{
    CGPoint contentOffset = CGPointMake(MAX(0, offset.x),
                                        MAX(0, offset.y));
    
    CGFloat itemHeight = self.itemSize.height + self.itemSpacing;
    
    CGFloat firstRow = MAX(0, (int)(contentOffset.y / itemHeight) - 1);
    
    CGFloat lastRow = ceil((contentOffset.y + self.gridBounds.size.height) / itemHeight);
    
    NSInteger firstPosition = firstRow * self.numberOfItemsPerRow;
    NSInteger lastPosition  = ((lastRow + 1) * self.numberOfItemsPerRow);
    return NSMakeRange(firstPosition, (lastPosition - firstPosition));
}

@end


//////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark - Horizontal strategy implementation
//////////////////////////////////////////////////////////////

@implementation ESGridViewLayoutHorizontalStrategy

@synthesize numberOfItemsPerColumn = _numberOfItemsPerColumn;

+ (BOOL)requiresEnablingPaging
{
    return NO;
}

- (id)init
{
    if ((self = [super init]))
    {
        _type = ESGridViewLayoutHorizontal;
    }
    
    return self;
}

- (void)rebaseWithItemCount:(NSInteger)count insideOfBounds:(CGRect)bounds
{
    _itemCount  = count;
    _gridBounds = bounds;
    
    CGRect actualBounds = CGRectMake(0,
                                     0,
                                     bounds.size.width  - self.minEdgeInsets.right - self.minEdgeInsets.left,
                                     bounds.size.height - self.minEdgeInsets.top   - self.minEdgeInsets.bottom);
    
    _numberOfItemsPerColumn = 1;
    
    while ((_numberOfItemsPerColumn + 1) * (self.itemSize.height + self.itemSpacing) - self.itemSpacing <= actualBounds.size.height)
    {
        _numberOfItemsPerColumn++;
    }
    
    NSInteger numberOfColumns = ceil(self.itemCount / (1.0 * self.numberOfItemsPerColumn));
    
    CGSize actualContentSize = CGSizeMake(ceil(numberOfColumns * (self.itemSize.width + self.itemSpacing)) - self.itemSpacing,
                                          ceil(MIN(self.itemCount, self.numberOfItemsPerColumn) * (self.itemSize.height + self.itemSpacing)) - self.itemSpacing);
    
    [self setEdgeAndContentSizeFromAbsoluteContentSize:actualContentSize];
}

- (CGPoint)originForItemAtPosition:(NSInteger)position
{
    CGPoint origin = CGPointZero;
    if (self.numberOfItemsPerColumn > 0 && position >= 0)
    {
        NSUInteger col = position / self.numberOfItemsPerColumn;
        NSUInteger row = position % self.numberOfItemsPerColumn;
        origin = CGPointMake(col * (self.itemSize.width + self.itemSpacing) + self.edgeInsets.left,
                             row * (self.itemSize.height + self.itemSpacing) + self.edgeInsets.top);
    }
    
    return origin;
}

- (NSInteger)itemPositionFromLocation:(CGPoint)location
{
    CGPoint relativeLocation = CGPointMake(location.x - self.edgeInsets.left,
                                           location.y - self.edgeInsets.top);
    
    int col = (int) (relativeLocation.x / (self.itemSize.width + self.itemSpacing));
    int row = (int) (relativeLocation.y / (self.itemSize.height + self.itemSpacing));
    
    int position = row + col * self.numberOfItemsPerColumn;
    
    if (position >= [self itemCount] || position < 0)
    {
        position = INVALID_POSITION;
    }
    else
    {
        CGPoint itemOrigin = [self originForItemAtPosition:position];
        CGRect itemFrame = CGRectMake(itemOrigin.x,
                                      itemOrigin.y,
                                      self.itemSize.width,
                                      self.itemSize.height);
        
        if (!CGRectContainsPoint(itemFrame, location))
        {
            position = INVALID_POSITION;
        }
    }
    
    return position;
}

- (NSRange)rangeOfPositionsInBoundsFromOffset:(CGPoint)offset
{
    CGPoint contentOffset = CGPointMake(MAX(0, offset.x),
                                        MAX(0, offset.y));
    
    CGFloat itemWidth = self.itemSize.width + self.itemSpacing;
    
    CGFloat firstCol = MAX(0, (int)(contentOffset.x / itemWidth) - 1);
    
    CGFloat lastCol = ceil((contentOffset.x + self.gridBounds.size.width) / itemWidth);
    
    NSInteger firstPosition = firstCol * self.numberOfItemsPerColumn;
    NSInteger lastPosition  = ((lastCol + 1) * self.numberOfItemsPerColumn);
    
    return NSMakeRange(firstPosition, (lastPosition - firstPosition));
}

@end



//////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark - HorizontalPaged strategy implementation
//////////////////////////////////////////////////////////////

@implementation ESGridViewLayoutHorizontalPagedStrategy

@synthesize numberOfItemsPerPage = _numberOfItemsPerPage;
@synthesize numberOfItemsPerRow  = _numberOfItemsPerRow;
@synthesize numberOfPages        = _numberOfPages;

+ (BOOL)requiresEnablingPaging
{
    return YES;
}

- (void)rebaseWithItemCount:(NSInteger)count insideOfBounds:(CGRect)bounds
{
    [super rebaseWithItemCount:count insideOfBounds:bounds];
    
    _numberOfItemsPerRow = 1;
    
    NSInteger gridContentMaxWidth = self.gridBounds.size.width - self.minEdgeInsets.right - self.minEdgeInsets.left;
    
    while ((self.numberOfItemsPerRow + 1) * (self.itemSize.width + self.itemSpacing) - self.itemSpacing <= gridContentMaxWidth)
    {
        _numberOfItemsPerRow++;
    }
    
    _numberOfItemsPerPage = _numberOfItemsPerRow * _numberOfItemsPerColumn;
    _numberOfPages = ceil(self.itemCount * 1.0 / self.numberOfItemsPerPage);
    
    CGSize onePageSize = CGSizeMake(self.numberOfItemsPerRow * (self.itemSize.width + self.itemSpacing) - self.itemSpacing,
                                    self.numberOfItemsPerColumn * (self.itemSize.height + self.itemSpacing) - self.itemSpacing);
    
    if (self.centeredGrid)
    {
        NSInteger widthSpace, heightSpace;
        NSInteger top, left, bottom, right;
        
        widthSpace  = floor((self.gridBounds.size.width  - onePageSize.width)  / 2.0);
        heightSpace = floor((self.gridBounds.size.height - onePageSize.height) / 2.0);
        
        left   = MAX(widthSpace,  self.minEdgeInsets.left);
        right  = MAX(widthSpace,  self.minEdgeInsets.right);
        top    = MAX(heightSpace, self.minEdgeInsets.top);
        bottom = MAX(heightSpace, self.minEdgeInsets.bottom);
        
        _edgeInsets = UIEdgeInsetsMake(top, left, bottom, right);
    }
    else
    {
        _edgeInsets = self.minEdgeInsets;
    }
    
    _contentSize = CGSizeMake(bounds.size.width * self.numberOfPages,
                              bounds.size.height);
}

- (NSInteger)pageForItemAtIndex:(NSInteger)index
{
    return MAX(0, floor(index * 1.0 / self.numberOfItemsPerPage * 1.0));
}

- (CGPoint)originForItemAtColumn:(NSInteger)column row:(NSInteger)row page:(NSInteger)page
{
    CGPoint offset = CGPointMake(page * self.gridBounds.size.width,
                                 0);
    
    CGFloat x = column * (self.itemSize.width + self.itemSpacing) + self.edgeInsets.left;
    CGFloat y = row * (self.itemSize.height + self.itemSpacing) + self.edgeInsets.top;
    
    return CGPointMake(x + offset.x,
                       y + offset.y);
}

- (NSInteger)positionForItemAtColumn:(NSInteger)column row:(NSInteger)row page:(NSInteger)page
{
    return column + row * self.numberOfItemsPerRow + (page * self.numberOfItemsPerPage);
}

- (NSInteger)columnForItemAtPosition:(NSInteger)position
{
    position %= self.numberOfItemsPerPage;
    return position % self.numberOfItemsPerRow;;
}

- (NSInteger)rowForItemAtPosition:(NSInteger)position
{
    position %= self.numberOfItemsPerPage;
    return floor(position / self.numberOfItemsPerRow);
}

- (CGPoint)originForItemAtPosition:(NSInteger)position
{
    NSUInteger page = [self pageForItemAtIndex:position];
    
    position %= self.numberOfItemsPerPage;
    
    NSUInteger row = [self rowForItemAtPosition:position];
    NSUInteger column = [self columnForItemAtPosition:position];
    
    CGPoint origin = [self originForItemAtColumn:column row:row page:page];
    
    return origin;
}

- (NSInteger)itemPositionFromLocation:(CGPoint)location
{
    CGFloat page = 0;
    while ((page + 1) * self.gridBounds.size.width < location.x)
    {
        page++;
    }
    
    CGPoint originForFirstItemInPage = [self originForItemAtColumn:0 row:0 page:page];
    
    CGPoint relativeLocation = CGPointMake(location.x - originForFirstItemInPage.x,
                                           location.y - originForFirstItemInPage.y);
    
    int col = (int) (relativeLocation.x / (self.itemSize.width + self.itemSpacing));
    int row = (int) (relativeLocation.y / (self.itemSize.height + self.itemSpacing));
    
    int position = [self positionForItemAtColumn:col row:row page:page];
    
    if (position >= [self itemCount] || position < 0)
    {
        position = INVALID_POSITION;
    }
    else
    {
        CGPoint itemOrigin = [self originForItemAtPosition:position];
        CGRect itemFrame = CGRectMake(itemOrigin.x,
                                      itemOrigin.y,
                                      self.itemSize.width,
                                      self.itemSize.height);
        
        if (!CGRectContainsPoint(itemFrame, location))
        {
            position = INVALID_POSITION;
        }
    }
    
    return position;
}

- (NSRange)rangeOfPositionsInBoundsFromOffset:(CGPoint)offset
{
    CGPoint contentOffset = CGPointMake(MAX(0, offset.x),
                                        MAX(0, offset.y));
    
    NSInteger page = floor(contentOffset.x / self.gridBounds.size.width);
    
    NSInteger firstPosition = MAX(0, (page - 1) * self.numberOfItemsPerPage);
    NSInteger lastPosition  = MIN(firstPosition + 3 * self.numberOfItemsPerPage, self.itemCount);

    return NSMakeRange(firstPosition, (lastPosition - firstPosition));
}

@end


//////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark - HorizontalPagedLTR strategy implementation
//////////////////////////////////////////////////////////////

@implementation ESGridViewLayoutHorizontalPagedLTRStrategy

- (id)init
{
    if ((self = [super init]))
    {
        _type = ESGridViewLayoutHorizontalPagedLTR;
    }
    
    return self;
}

// Nothing to change, LTR is already the behavior of the base class

@end


//////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark - HorizontalPagedRTL strategy implementation
//////////////////////////////////////////////////////////////

@implementation ESGridViewLayoutHorizontalPagedTTBStrategy

- (id)init
{
    if ((self = [super init]))
    {
        _type = ESGridViewLayoutHorizontalPagedTTB;
    }
    
    return self;
}

- (NSInteger)positionForItemAtColumn:(NSInteger)column row:(NSInteger)row page:(NSInteger)page
{
    return row + column * self.numberOfItemsPerColumn + (page * self.numberOfItemsPerPage);
}

- (NSInteger)columnForItemAtPosition:(NSInteger)position
{
    position %= self.numberOfItemsPerPage;
    return floor(position / self.numberOfItemsPerColumn);
}

- (NSInteger)rowForItemAtPosition:(NSInteger)position
{
    position %= self.numberOfItemsPerPage;
    return position % self.numberOfItemsPerColumn;
}

@end




//////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark - VerticalPaged strategy implementation
//////////////////////////////////////////////////////////////

@implementation ESGridViewlayoutVerticalPagedStrategy

@synthesize numberOfItemsPerPage    = _numberOfItemsPerPage;
@synthesize numberOfItemsPerColumn  = _numberOfItemsPerColumn;
@synthesize numberOfPages           = _numberOfPages;

+ (BOOL)requiresEnablingPaging
{
    return YES;
}

- (void)rebaseWithItemCount:(NSInteger)count insideOfBounds:(CGRect)bounds
{
    [super rebaseWithItemCount:count insideOfBounds:bounds];
    
    
    _numberOfItemsPerColumn = 1;

    NSInteger gridContentMaxHeight = self.gridBounds.size.height - self.minEdgeInsets.top - self.minEdgeInsets.bottom;
        
    while ((self.numberOfItemsPerColumn + 1) * (self.itemSize.height + self.itemSpacing) - self.itemSpacing <= gridContentMaxHeight)
    {
        _numberOfItemsPerColumn++;
    }

    _numberOfItemsPerPage = _numberOfItemsPerRow * _numberOfItemsPerColumn;
    _numberOfPages = ceil(self.itemCount * 1.0 / self.numberOfItemsPerPage);

    CGSize onePageSize = CGSizeMake(self.numberOfItemsPerRow * (self.itemSize.width + self.itemSpacing) - self.itemSpacing,
                                    self.numberOfItemsPerColumn * (self.itemSize.height + self.itemSpacing) - self.itemSpacing);
    
    if (self.centeredGrid)
    {
        NSInteger widthSpace, heightSpace;
        NSInteger top, left, bottom, right;
        
        widthSpace  = floor((self.gridBounds.size.width  - onePageSize.width)  / 2.0);
        heightSpace = floor((self.gridBounds.size.height - onePageSize.height) / 2.0);
        
        left   = MAX(widthSpace,  self.minEdgeInsets.left);
        right  = MAX(widthSpace,  self.minEdgeInsets.right);
        top    = MAX(heightSpace, self.minEdgeInsets.top);
        bottom = MAX(heightSpace, self.minEdgeInsets.bottom);
        
        _edgeInsets = UIEdgeInsetsMake(top, left, bottom, right);
    }
    else
    {
        _edgeInsets = self.minEdgeInsets;
    }
    

    _contentSize = CGSizeMake(bounds.size.width,
                              bounds.size.height * self.numberOfPages);
}

- (NSInteger)pageForItemAtIndex:(NSInteger)index
{
    return MAX(0, floor(index * 1.0 / self.numberOfItemsPerPage * 1.0));
}

- (CGPoint)originForItemAtColumn:(NSInteger)column row:(NSInteger)row page:(NSInteger)page
{
    CGPoint offset = CGPointMake(0,
                                 page * self.gridBounds.size.height);
    CGFloat x = column * (self.itemSize.width + self.itemSpacing) + self.edgeInsets.left;
    CGFloat y = row * (self.itemSize.height + self.itemSpacing) + self.edgeInsets.top;

    return CGPointMake(x + offset.x,
                       y + offset.y);
}

- (NSInteger)positionForItemAtColumn:(NSInteger)column row:(NSInteger)row page:(NSInteger)page
{
    return column + row * self.numberOfItemsPerRow + (page * self.numberOfItemsPerPage);
}

- (NSInteger)columnForItemAtPosition:(NSInteger)position
{
    position %= self.numberOfItemsPerPage;
    return position % self.numberOfItemsPerRow;;
}

- (NSInteger)rowForItemAtPosition:(NSInteger)position
{
    position %= self.numberOfItemsPerPage;
    return floor(position / self.numberOfItemsPerRow);
}

- (CGPoint)originForItemAtPosition:(NSInteger)position
{
    NSUInteger page = [self pageForItemAtIndex:position];
    
    position %= self.numberOfItemsPerPage;
    
    NSUInteger row = [self rowForItemAtPosition:position];
    NSUInteger column = [self columnForItemAtPosition:position];
    
    CGPoint origin = [self originForItemAtColumn:column row:row page:page];
    
    return origin;
}

- (NSInteger)itemPositionFromLocation:(CGPoint)location
{
    CGFloat page = 0;
    while ((page + 1) * self.gridBounds.size.height < location.y)
    {
        page++;
    }
    
    CGPoint originForFirstItemInPage = [self originForItemAtColumn:0 row:0 page:page];
    
    CGPoint relativeLocation = CGPointMake(location.x - originForFirstItemInPage.x,
                                           location.y - originForFirstItemInPage.y);
    
    int col = (int) (relativeLocation.x / (self.itemSize.width + self.itemSpacing));
    int row = (int) (relativeLocation.y / (self.itemSize.height + self.itemSpacing));
    
    int position = [self positionForItemAtColumn:col row:row page:page];
    
    if (position >= [self itemCount] || position < 0)
    {
        position = INVALID_POSITION;
    }
    else
    {
        CGPoint itemOrigin = [self originForItemAtPosition:position];
        CGRect itemFrame = CGRectMake(itemOrigin.x,
                                      itemOrigin.y,
                                      self.itemSize.width,
                                      self.itemSize.height);
        if (!CGRectContainsPoint(itemFrame, location))
        {
            position = INVALID_POSITION;
        }
    }
    
    return position;
}

- (NSRange)rangeOfPositionsInBoundsFromOffset:(CGPoint)offset
{
    CGPoint contentOffset = CGPointMake(MAX(0, offset.x),
                                        MAX(0, offset.y));
    
    NSInteger page = floor(contentOffset.y / self.gridBounds.size.height);
    
    NSInteger firstPosition = MAX(0, (page - 1) * self.numberOfItemsPerPage);
    NSInteger lastPosition  = MIN(firstPosition + 3 * self.numberOfItemsPerPage, self.itemCount);
    
    return NSMakeRange(firstPosition, (lastPosition - firstPosition));
}

@end


//////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark - HorizontalPagedLTR strategy implementation
//////////////////////////////////////////////////////////////

@implementation ESGridViewlayoutVerticalPagedLTRStrategy

- (id)init
{
    if ((self = [super init]))
    {
        _type = ESGridViewLayoutVerticalPagedLTR;
    }
    
    return self;
}

// Nothing to change, LTR is already the behavior of the base class

@end


//////////////////////////////////////////////////////////////
#pragma mark -
#pragma mark - HorizontalPagedRTL strategy implementation
//////////////////////////////////////////////////////////////

@implementation ESGridViewlayoutVerticalPagedTTBStrategy

- (id)init
{
    if ((self = [super init]))
    {
        _type = ESGridViewLayoutVerticalPagedTTB;
    }
    
    return self;
}

- (NSInteger)positionForItemAtColumn:(NSInteger)column row:(NSInteger)row page:(NSInteger)page
{
    return row + column * self.numberOfItemsPerColumn + (page * self.numberOfItemsPerPage);
}

- (NSInteger)columnForItemAtPosition:(NSInteger)position
{
    position %= self.numberOfItemsPerPage;
    return floor(position / self.numberOfItemsPerColumn);
}

- (NSInteger)rowForItemAtPosition:(NSInteger)position
{
    position %= self.numberOfItemsPerPage;
    return position % self.numberOfItemsPerColumn;
}
@end