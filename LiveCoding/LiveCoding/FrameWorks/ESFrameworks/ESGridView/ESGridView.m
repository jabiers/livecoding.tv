//
//  ESGridView.m
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 27..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import "ESGridView.h"
#import <QuartzCore/QuartzCore.h>

@interface ESGridView () <UIScrollViewDelegate> {
    UIPinchGestureRecognizer *_pinchGesture;
    UITapGestureRecognizer *_tapGesture;
    
    NSInteger _numberTotalItems;
    NSMutableSet *_reusableCells;
    
    CGPoint _minPossibleContentOffset;
    CGPoint _maxPossibleContentOffset;
    
    UIInterfaceOrientation _orient;
    
}

@property (nonatomic, readonly) BOOL itemsSubviewsCacheIsValid;
@property (nonatomic, strong) NSArray *itemSubviewsCache;

@property (atomic) NSInteger firstPositionLoaded;
@property (atomic) NSInteger lastPositionLoaded;


@end

@implementation ESGridView
@synthesize dataSource = _datasource;
@synthesize delegate = __delegate;
@synthesize gridViewLayoutType = _gridViewLayoutType;

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self initialize];
        
    }
    return self;
}


#pragma mark - Initialize Method
-(void)initialize {
    _currentPage = 1;
    _reusableCells = [NSMutableSet set];
    self.layoutStrategy = [ESGridViewLayoutStrategyFactory strategyFromType:ESGridViewLayoutVerticalPagedLTR];
    self.pagingEnabled = YES; //default YES;
    [self setBackgroundColor:[UIColor whiteColor]]; //default white color
    [self setScrollIndicatorInsets:UIEdgeInsetsZero];
    [self setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
    _defaultGridSize = kDefaultSize;
    [self setCurrentGridSize:_defaultGridSize]; //default 180, 130;
    [self setMinimumGridZoomScale:1.0]; // default 1.0
    [self setMaximumGridZoomScale:1.0]; // default 1.0
    [self setAllowGridScaling:NO]; // default no;
    _margin = 5; //default 5
    _minEdgeInset = UIEdgeInsetsMake(5, 5, 5, 5); // default inset 5,5,5,5
    
    _orient = (UIInterfaceOrientation)[[UIDevice currentDevice] orientation];
    _tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(gestureRecognizeAction:)];
    _tapGesture.numberOfTapsRequired = 1;
    _tapGesture.numberOfTouchesRequired = 1;
    _tapGesture.cancelsTouchesInView = NO;
    
    [super setDelegate:self];
    [self addGestureRecognizer:_tapGesture];
    
    [self setEditing:NO];
}


#pragma mark - setter / getter method


-(void)setAllowGridScaling:(BOOL)allowGridScaling {
    _allowGridScaling = allowGridScaling;
    
    if (!_pinchGesture) {
        _pinchGesture = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(gestureRecognizeAction:)];
    }
    
    if (_allowGridScaling) {
        if (![[self gestureRecognizers] containsObject:_pinchGesture])
            [self addGestureRecognizer:_pinchGesture];
        
    } else {
        if ([[self gestureRecognizers] containsObject:_pinchGesture])
            [self removeGestureRecognizer:_pinchGesture];
    }
}


-(void)setDataSource:(id<ESGridViewDataSource>)dataSource {
    if (_datasource != dataSource) {
        _datasource = dataSource;
        //        [self setNeedsLayout];
        [self reloadData];
    }
    //    [self setNeedsLayout];
}

-(void)setCurrentGridSize:(CGSize)currentItemSize {
    if (!CGSizeEqualToSize(_currentGridSize, currentItemSize)) {
        if ([__delegate respondsToSelector:@selector(gridView:willChangeWithPreGridSize:)])
            [__delegate gridView:self willChangeWithPreGridSize:_currentGridSize];
        
        _currentGridSize = currentItemSize;
        [self reloadData];
        
        if ([__delegate respondsToSelector:@selector(gridView:didChangeWithAfterGridSize:)])
            [__delegate gridView:self didChangeWithAfterGridSize:_currentGridSize];
        
    }
}


-(void)setCurrentPage:(NSInteger)toPage {
    [self setCurrentPage:toPage withAnimation:NO];
}

-(void)setCurrentPage:(NSInteger)toPage withAnimation:(BOOL)animation {
    
    CGPoint dest = CGPointZero;
    if ([self gridViewLayoutType] == ESGridViewLayoutType_Horizonal) {
        dest = CGPointMake((toPage - 1) * self.frame.size.width, 0);
    } else if ([self gridViewLayoutType] == ESGridViewLayoutType_Vertical) {
        dest = CGPointMake((toPage - 1) * self.frame.size.height, 0);
    }
    
    [self setContentOffset:dest animated:animation];
}

-(void)setLayoutStrategyType:(ESGridViewLayoutStrategyType)type {
    self.layoutStrategy = [ESGridViewLayoutStrategyFactory strategyFromType:type];
    switch (type) {
        case ESGridViewLayoutHorizontalPagedLTR:
        case ESGridViewLayoutHorizontalPagedTTB:
        case ESGridViewLayoutVerticalPagedLTR:
        case ESGridViewLayoutVerticalPagedTTB:
            [self setPagingEnabled:YES];
            break;
        case ESGridViewLayoutHorizontal:
        case ESGridViewLayoutVertical:
            [self setPagingEnabled:NO];
            break;
    }
    [self reloadData];
}

- (void)setSubviewsCacheAsInvalid
{
    _itemsSubviewsCacheIsValid = NO;
}

-(void)setMaximumGridZoomScale:(float)maximumGridZoomScale {
    
    if (_maximumGridZoomScale != maximumGridZoomScale) {
        [self setAllowGridScaling:YES];
    }
    _maximumGridZoomScale = maximumGridZoomScale;
}

-(void)setMinimumGridZoomScale:(float)minimumGridZoomScale {
    
    if (_minimumGridZoomScale != minimumGridZoomScale) {
        [self setAllowGridScaling:YES];
    }
    _minimumGridZoomScale = minimumGridZoomScale;
}

-(ESGridViewLayoutType)gridViewLayoutType {
    if ([self.layoutStrategy type] == ESGridViewLayoutHorizontal ||
        [self.layoutStrategy type] == ESGridViewLayoutHorizontalPagedLTR ||
        [self.layoutStrategy type] == ESGridViewLayoutHorizontalPagedLTR ) {
        _gridViewLayoutType = ESGridViewLayoutType_Horizonal;
    } else if ([self.layoutStrategy type] == ESGridViewLayoutVertical ||
               [self.layoutStrategy type] == ESGridViewLayoutVerticalPagedLTR ||
               [self.layoutStrategy type] == ESGridViewLayoutVerticalPagedTTB ) {
        _gridViewLayoutType = ESGridViewLayoutType_Vertical;
    }
    
    return _gridViewLayoutType;
}

-(NSInteger)numberOfItemsPerPage {
    return (NSInteger)[((ESGridViewLayoutStrategyBase*)self.layoutStrategy) performSelector:@selector(numberOfItemsPerPage)];
}

#pragma mark - Draw Layout
#define ROOTVIEWCONTROLLER [[[UIApplication sharedApplication] keyWindow] rootViewController]

-(void)layoutSubviews {
    [super layoutSubviews];
    
    //    if (_orient != [ROOTVIEWCONTROLLER interfaceOrientation]) {
    //        _orient = [ROOTVIEWCONTROLLER interfaceOrientation];
    //        [self reloadData];
    //    }
    
    //    [self recomputeSizeAnimated:NO];
    //    [self loadRequiredGrids];
    
    
    [self recomputeSizeAnimated:NO];
    [self loadRequiredGrids];
    
    [self setSubviewsCacheAsInvalid];
    
}

- (void)recomputeSizeAnimated:(BOOL)animated
{
    [self.layoutStrategy setupItemSize:_currentGridSize andItemSpacing:_margin withMinEdgeInsets:_minEdgeInset andCenteredGrid:YES];
    [self.layoutStrategy rebaseWithItemCount:_numberTotalItems insideOfBounds:self.bounds];
    
    CGSize contentSize = [self.layoutStrategy contentSize];
    
    if ([self gridViewLayoutType] == ESGridViewLayoutType_Horizonal) {
        _totalPage = contentSize.width / self.frame.size.width;
    } else if ([self gridViewLayoutType] == ESGridViewLayoutType_Vertical) {
        _totalPage = contentSize.height / self.frame.size.height;
    }
    
    _minPossibleContentOffset = CGPointMake(0, 0);
    _maxPossibleContentOffset = CGPointMake(contentSize.width - self.bounds.size.width + self.contentInset.right,
                                            contentSize.height - self.bounds.size.height + self.contentInset.bottom);
    
    BOOL shouldUpdateScrollviewContentSize = !CGSizeEqualToSize(self.contentSize, contentSize);
    
    if (shouldUpdateScrollviewContentSize)
    {
        if (animated)
        {
            [UIView animateWithDuration:0.25
                                  delay:0
                                options:UIViewAnimationOptionAutoreverse
                             animations:^{
                                 self.contentSize = contentSize;
                             }
                             completion:nil];
        }
        else
        {
            self.contentSize = contentSize;
        }
    }
}

-(void)loadRequiredGrids {
    NSRange rangeOfPositions = [self.layoutStrategy rangeOfPositionsInBoundsFromOffset: self.contentOffset];
    NSRange loadedPositionsRange = NSMakeRange(self.firstPositionLoaded, self.lastPositionLoaded - self.firstPositionLoaded);
    
    // calculate new position range
    self.firstPositionLoaded = self.firstPositionLoaded == INVALID_POSITION ? rangeOfPositions.location : MIN(self.firstPositionLoaded, (NSInteger)rangeOfPositions.location);
    self.lastPositionLoaded  = self.lastPositionLoaded == INVALID_POSITION ? NSMaxRange(rangeOfPositions) : MAX(self.lastPositionLoaded, (NSInteger)(rangeOfPositions.length + rangeOfPositions.location));
    
    // remove now invisible items
    [self setSubviewsCacheAsInvalid];
    [self cleanupUnseenItems];
    
    // add new cells
    BOOL forceLoad = self.firstPositionLoaded == INVALID_POSITION || self.lastPositionLoaded == INVALID_POSITION;
    NSInteger positionToLoad;
    for (NSUInteger i = 0; i < rangeOfPositions.length; i++)
    {
        positionToLoad = i + rangeOfPositions.location;
        
        if ((forceLoad || !NSLocationInRange(positionToLoad, loadedPositionsRange)) && positionToLoad < _numberTotalItems)
        {
            if (![self cellForGridAtIndex:positionToLoad])
            {
                ESGridViewCell *cell = [self newItemSubViewForPosition:positionToLoad];
                [cell setAlpha:0.0f];
                [self addSubview:cell];
                
                [UIView beginAnimations:@"FadeAnimations" context:nil];
                [UIView setAnimationDuration:0.4];
                
                [cell setAlpha:1.0f];
                
                [UIView commitAnimations];
            }
        }
    }
}

- (void)cleanupUnseenItems
{
    NSRange rangeOfPositions = [self.layoutStrategy rangeOfPositionsInBoundsFromOffset: self.contentOffset];
    ESGridViewCell *cell;
    
    if ((NSInteger)rangeOfPositions.location > self.firstPositionLoaded)
    {
        for (NSInteger i = self.firstPositionLoaded; i < (NSInteger)rangeOfPositions.location; i++)
        {
            cell = [self cellForGridAtIndex:i];
            if(cell)
            {
                [self queueReusableCell:cell];
                [cell removeFromSuperview];
            }
        }
        
        self.firstPositionLoaded = rangeOfPositions.location;
        [self setSubviewsCacheAsInvalid];
    }
    
    if ((NSInteger)NSMaxRange(rangeOfPositions) < self.lastPositionLoaded)
    {
        for (NSInteger i = NSMaxRange(rangeOfPositions); i <= self.lastPositionLoaded; i++)
        {
            cell = [self cellForGridAtIndex:i];
            if(cell)
            {
                [self queueReusableCell:cell];
                [cell removeFromSuperview];
            }
        }
        
        self.lastPositionLoaded = NSMaxRange(rangeOfPositions);
        [self setSubviewsCacheAsInvalid];
    }
}

- (void)reloadData
{
    [_reusableCells removeAllObjects];
    [[self itemSubviews] enumerateObjectsUsingBlock:^(id obj, NSUInteger index, BOOL *stop)
     {
         if ([obj isKindOfClass:[ESGridViewCell class]])
         {
             [(UIView *)obj removeFromSuperview];
             [self queueReusableCell:(ESGridViewCell *)obj];
         }
     }];
    
    self.firstPositionLoaded = INVALID_POSITION;
    self.lastPositionLoaded  = INVALID_POSITION;
    
    [self setSubviewsCacheAsInvalid];
    NSUInteger numberItems = 0;
    if ([self.dataSource respondsToSelector:@selector(numberOfTotalGrid:)]) {
        numberItems = [self.dataSource numberOfTotalGrid:self];
    }
#if DEBUG
    else if (self.dataSource) {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"DEBUG" message:@"override -(void)numberOfTotalGrid: Method" delegate:nil cancelButtonTitle:nil otherButtonTitles:@"ok", nil];
        [alert show];
    }
#endif
    
    _numberTotalItems = numberItems;
    //
    [self recomputeSizeAnimated:NO];
    //    [self loadRequiredGrids];
    //
    //    [self setSubviewsCacheAsInvalid];
    [self setNeedsLayout];
}

-(void)reloadDataAtIndex:(NSInteger)index {
    ESGridViewCell *cell = [self cellForGridAtIndex:index];
    [cell removeFromSuperview];
    
    cell = [self newItemSubViewForPosition:index];
    [self addSubview:cell];
}

#pragma mark - method about cell

- (ESGridViewCell *)cellForGridAtIndex:(NSInteger)index
{
    ESGridViewCell *view = nil;
    
    for (ESGridViewCell *v in [self itemSubviews])
    {
        if (v.tag == index + kTagOffset)
        {
            view = v;
            break;
        }
    }
    return view;
}

- (ESGridViewCell *)newItemSubViewForPosition:(NSInteger)position
{
    ESGridViewCell *cell = [self.dataSource gridView:self cellForIndex:position];
    [cell prepareReload];
    CGPoint origin = [self.layoutStrategy originForItemAtPosition:position];
    CGRect frame = CGRectMake(origin.x, origin.y, _currentGridSize.width, _currentGridSize.height);
    
    //    // To make sure the frame is not animated
    [self applyWithoutAnimation:^{
        cell.frame = frame;
        cell.contentView.frame = cell.bounds;
    }];
    
    cell.tag = position + kTagOffset;
    
    __es_weak ESGridView *weakSelf = self;
    
    cell.deleteBlock = ^(ESGridViewCell *aCell)
    {
        NSInteger index = [weakSelf positionForItemSubview:aCell];
        if (index != INVALID_POSITION)
        {
            BOOL canDelete = YES;
            if ([weakSelf.dataSource respondsToSelector:@selector(gridView:canDeleteItemAtIndex:)])
            {
                canDelete = [weakSelf.dataSource gridView:weakSelf canDeleteItemAtIndex:index];
            }
            
            if (canDelete && [weakSelf.delegate respondsToSelector:@selector(gridView:processDeleteActionForItemAtIndex:)])
            {
                [weakSelf.delegate gridView:weakSelf processDeleteActionForItemAtIndex:index];
            }
        }
    };
    
    return cell;
}

- (void)applyWithoutAnimation:(void (^)(void))animations
{
    if (animations)
    {
        [CATransaction begin];
        [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
        animations();
        [CATransaction commit];
    }
}

- (void)setEditing:(BOOL)editing
{
    [self setEditing:editing animated:NO];
    if ([self.delegate respondsToSelector:@selector(gridView:changedEdit:)]) {
        [self.delegate gridView:self changedEdit:editing];
    }
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated
{
    if ([self.delegate respondsToSelector:@selector(gridView:processDeleteActionForItemAtIndex:)]
        && ((self.isEditing && !editing) || (!self.isEditing && editing)))
    {
        for (ESGridViewCell *cell in [self itemSubviews])
        {
            NSInteger index = [self positionForItemSubview:cell];
            if (index != INVALID_POSITION)
            {
                BOOL allowEdit = editing && [self.dataSource respondsToSelector:@selector(gridView:canDeleteItemAtIndex:)]?[self.dataSource gridView:self canDeleteItemAtIndex:index]:NO;
                [cell setEditing:allowEdit animated:animated];
            }
        }
        _editing = editing;
    }
    
    if (_editing) {
        [self removeGestureRecognizer:_tapGesture];
    } else {
        [self addGestureRecognizer:_tapGesture];
    }
}

- (NSInteger)positionForItemSubview:(ESGridViewCell *)view
{
    return view.tag >= kTagOffset ? view.tag - kTagOffset : INVALID_POSITION;
}

#pragma mark - cache & queue methods

- (NSArray *)itemSubviews
{
    NSArray *subviews = nil;
    
    if (self.itemsSubviewsCacheIsValid)
    {
        subviews = [self.itemSubviewsCache copy];
    }
    else
    {
        @synchronized(self)
        {
            NSMutableArray *itemSubViews = [[NSMutableArray alloc] initWithCapacity:_numberTotalItems];
            
            for (UIView * v in [self subviews])
            {
                if ([v isKindOfClass:[ESGridViewCell class]])
                {
                    [itemSubViews addObject:v];
                }
            }
            
            subviews = itemSubViews;
            
            self.itemSubviewsCache = [subviews copy];
            _itemsSubviewsCacheIsValid = YES;
        }
        
    }
    return subviews;
}


- (void)queueReusableCell:(ESGridViewCell *)cell
{
    if (cell)
    {
        [cell prepareQueueForReuse];
        cell.alpha = 1;
        cell.backgroundColor = [UIColor clearColor];
        [_reusableCells addObject:cell];
    }
}

- (ESGridViewCell *)dequeueReusableCell
{
    ESGridViewCell *cell = [_reusableCells anyObject];
    if (cell)
    {
        [cell prepareDeQueueForReuse];
        [_reusableCells removeObject:cell];
    }
    
    return cell;
}

- (ESGridViewCell *)dequeueReusableCellWithIdentifier:(NSString *)identifier
{
    ESGridViewCell *cell = nil;
    
    for (ESGridViewCell *reusableCell in [_reusableCells allObjects])
    {
        if ([reusableCell.reuseIdentifier isEqualToString:identifier])
        {
            [cell prepareDeQueueForReuse];
            cell = reusableCell;
            break;
        }
    }
    
    if (cell)
    {
        [_reusableCells removeObject:cell];
    }
    
    return cell;
}


#pragma mark - gesture Recognize Method

-(void)gestureRecognizeAction:(UIGestureRecognizer*)ges {
    
    if ( _pinchGesture == ges ) {
        
        static CGSize beginSize;
        if ([ges state] == UIGestureRecognizerStateBegan) {
            beginSize = _currentGridSize;
        } else if ([ges state] == UIGestureRecognizerStateChanged) {
            CGSize size = _currentGridSize;
            size.width = MIN(MAX(beginSize.width * ((UIPinchGestureRecognizer*)ges).scale, _defaultGridSize.width * _minimumGridZoomScale), _defaultGridSize.width * _maximumGridZoomScale);
            size.height = MIN(MAX(beginSize.height * ((UIPinchGestureRecognizer*)ges).scale, _defaultGridSize.height * _minimumGridZoomScale), _defaultGridSize.height * _maximumGridZoomScale);
            
            if (!CGSizeEqualToSize(size, self.currentGridSize)) {
                
                [self setCurrentGridSize:size];
                
            }
        } else if ([ges state] == UIGestureRecognizerStateEnded) {
            beginSize = CGSizeZero;
        }
    } else if ( _tapGesture == ges ) {
        CGPoint locationTouch = [_tapGesture locationInView:self];
        NSInteger position = [self.layoutStrategy itemPositionFromLocation:locationTouch];
        if (position != INVALID_POSITION)
        {
            
            if ([__delegate respondsToSelector:@selector(gridView:shouldSelectAtIndex:)])
                if (![__delegate gridView:self shouldSelectAtIndex:position])
                    return;
            
            if (![[self cellForGridAtIndex:position] enabledTouch])
                return;
            
            if ([__delegate respondsToSelector:@selector(gridView:willSelectAtIndex:)])
                [__delegate gridView:self willSelectAtIndex:position];
            
            [self cellForGridAtIndex:position].highLighted = NO;
            
            if ([__delegate respondsToSelector:@selector(gridView:didSelectedAtIndex:)])
                [__delegate gridView:self didSelectedAtIndex:position];
            
        }
        else
        {
            if ([__delegate respondsToSelector:@selector(gridView:didTouchInEmptySpace:)])
                [__delegate gridView:self didTouchInEmptySpace:[NSNull null]];
        }
    }
}

#pragma mark - UIScrollView Delegate

-(void)scrollViewDidScroll:(UIScrollView *)scrollView {
    [self caculateCurrentPage];
}

-(void)caculateCurrentPage {
    CGRect r;
    r.origin = self.contentOffset;
    r.size = self.frame.size;
    NSInteger page = 0;
    
    if ([self gridViewLayoutType] == ESGridViewLayoutType_Horizonal) {
        page = CGRectGetMidX(r) / self.frame.size.width + 1;
    } else if ([self gridViewLayoutType] == ESGridViewLayoutType_Vertical) {
        page = CGRectGetMidY(r) / self.frame.size.height + 1;
    }
    
    if ([__delegate respondsToSelector:@selector(gridView:willContentOffsetChange:)]) {
        [__delegate gridView:self willContentOffsetChange:self.contentOffset];
    }
    
    if ([__delegate respondsToSelector:@selector(gridView:didContentOffsetChange:)])
        [__delegate gridView:self didContentOffsetChange:self.contentOffset];
    
    if ( _currentPage !=  page ) {
        
        if ([__delegate respondsToSelector:@selector(gridView:willChangePage:)])
            [__delegate gridView:self willChangePage:_currentPage];
        
        _currentPage = page;
        
        if ([__delegate respondsToSelector:@selector(gridView:didChangedPage:)])
            [__delegate gridView:self didChangedPage:_currentPage];
        
    }
    //
}

@end
