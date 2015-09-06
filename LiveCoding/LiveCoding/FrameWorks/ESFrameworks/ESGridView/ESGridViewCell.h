//
//  ESGridViewCell.h
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 27..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ESGridView-Constants.h"
/** GridView에서 기본으로 사용될 GridViewCell
프러퍼티
 contentView 내용을 담는 뷰
 reuseIdentifier 재사용에 사용될 identifier
 highLighted 셀 선택시 하이라이트 효과를 위한 프러퍼티
 

일반 메쏘드
 initWithSize: 파라메터로 받은 사이즈 값으로 초기화
 prepareQueueForReuse 큐에 들어가기전 초기화 하는 메쏘드
 prepareDeQueueForReuse 큐에서 나오기전 초기화 하는 메쏘드
 
 */
//author Dae-hyun Kim

@interface ESGridViewCell : UIView
typedef void (^ESGridViewCellDeleteBlock)(ESGridViewCell*);

@property (nonatomic, strong) UIView *contentView;
@property (nonatomic, strong) NSString *reuseIdentifier;
@property (nonatomic, getter = isHighlighted) BOOL highLighted;
@property (nonatomic, assign) BOOL enabledTouch;

@property (nonatomic, getter=isEditing) BOOL editing;
@property (nonatomic, copy) ESGridViewCellDeleteBlock deleteBlock;
@property (nonatomic, es_weak) UIButton *deleteButton;
@property (nonatomic, strong) UIImage *deleteButtonIcon;
@property (nonatomic) CGPoint deleteButtonOffset;

@property (nonatomic, readonly, getter=isInShakingMode) BOOL inShakingMode;
@property (nonatomic, readonly, getter=isInFullSizeMode) BOOL inFullSizeMode;

-(id)initWithSize:(CGSize)size;
-(void)prepareQueueForReuse;
-(void)prepareDeQueueForReuse;
-(void)initialize;
-(void)setEditing:(BOOL)editing;
-(void)setEditing:(BOOL)editing animated:(BOOL)animated;
-(void)prepareReload;
@end
