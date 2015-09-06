//
//  ESGridViewCell.m
//  gridview
//
//  Created by Daehyun Kim on 13. 3. 27..
//  Copyright (c) 2013년 Daehyun Kim. All rights reserved.
//

#import "ESGridViewCell.h"
#import <QuartzCore/QuartzCore.h>

@interface UIView (ESGridViewAdditions_Privates)


@end

@implementation UIView (ESGridViewAdditions)

- (void)shakeStatus:(BOOL)enabled
{
    if (enabled)
    {
        CGFloat rotation = 0.08;
        
        CABasicAnimation *shake = [CABasicAnimation animationWithKeyPath:@"transform"];
        shake.duration = 0.08;
        shake.autoreverses = YES;
        shake.repeatCount  = MAXFLOAT;
        shake.removedOnCompletion = NO;
        shake.fromValue = [NSValue valueWithCATransform3D:CATransform3DRotate(self.layer.transform,-rotation, 0.0 ,0.0 ,1.0)];
        shake.toValue   = [NSValue valueWithCATransform3D:CATransform3DRotate(self.layer.transform, rotation, 0.0 ,0.0 ,1.0)];
        
        [self.layer addAnimation:shake forKey:@"shakeAnimation"];
    }
    else
    {
        [self.layer removeAnimationForKey:@"shakeAnimation"];
    }
}
@end

@implementation ESGridViewCell


-(id)initWithSize:(CGSize)size {
    CGRect r = CGRectZero;
    r.size = size;
    return [self initWithFrame:r];
}
- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame]) {
        [self initialize];
    }
    return self;
}

#pragma mark - initialize Method

-(void)initialize {
    self.editing = NO;

    [self setEnabledTouch:YES];
    
    UIButton *deleteButton = [UIButton buttonWithType:UIButtonTypeCustom];
    self.deleteButton = deleteButton;
    [self.deleteButton setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
    self.deleteButtonIcon = nil;
    self.deleteButtonOffset = CGPointMake(-5, -5);
    self.deleteButton.alpha = 0;
    [self addSubview:deleteButton];
    [deleteButton addTarget:self action:@selector(actionDelete) forControlEvents:UIControlEventTouchUpInside];

}


-(void)prepareQueueForReuse {
    
}

-(void)prepareDeQueueForReuse {
    
}

-(void)prepareReload {
    
}

#pragma mark - setter / getter Methods

- (void)setContentView:(UIView *)contentView
{
    [self shake:NO];
    [self.contentView removeFromSuperview];
    
    if(self.contentView)
    {
        contentView.frame = self.contentView.frame;
    }
    else
    {
        contentView.frame = self.bounds;
    }
    
    _contentView = contentView;
    
    self.contentView.autoresizingMask = UIViewAutoresizingNone;
    [self addSubview:self.contentView];
    
    [self bringSubviewToFront:self.deleteButton];
    
}

- (void)setDeleteButtonOffset:(CGPoint)offset
{
    self.deleteButton.frame = CGRectMake(offset.x,
                                         offset.y,
                                         self.deleteButton.frame.size.width,
                                         self.deleteButton.frame.size.height);
}

- (CGPoint)deleteButtonOffset
{
    return self.deleteButton.frame.origin;
}

- (void)setDeleteButtonIcon:(UIImage *)deleteButtonIcon
{
    [self.deleteButton setImage:deleteButtonIcon forState:UIControlStateNormal];
    
    if (deleteButtonIcon)
    {
        self.deleteButton.frame = CGRectMake(self.deleteButton.frame.origin.x,
                                             self.deleteButton.frame.origin.y,
                                             deleteButtonIcon.size.width,
                                             deleteButtonIcon.size.height);
        
        [self.deleteButton setTitle:nil forState:UIControlStateNormal];
        [self.deleteButton setBackgroundColor:[UIColor clearColor]];
    }
    else
    {
        self.deleteButton.frame = CGRectMake(self.frame.size.width - 35,
                                             self.frame.size.height - 35,
                                             35,
                                             35);
        
        [self.deleteButton setTitle:@"X" forState:UIControlStateNormal];
        [self.deleteButton setBackgroundColor:[UIColor clearColor]];
    }
}

- (void)setEditing:(BOOL)editing
{
    [self setEditing:editing animated:NO];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated
{
    if (editing != _editing) {
        _editing = editing;
        if (animated) {
            [UIView animateWithDuration:0.2f
                                  delay:0.f
                                options:UIViewAnimationOptionAllowUserInteraction | UIViewAnimationCurveEaseOut
                             animations:^{
                                 self.deleteButton.alpha = editing ? 1.f : 0.f;
                             }
                             completion:nil];
        }else {
            self.deleteButton.alpha = editing ? 1.f : 0.f;
        }
		
        self.contentView.userInteractionEnabled = !editing;
        [self shakeStatus:editing];
    }
}


- (void)shake:(BOOL)on
{
    if ((on && !self.inShakingMode) || (!on && self.inShakingMode))
    {
        [self.contentView shakeStatus:on];
        _inShakingMode = on;
    }
}


//////////////////////////////////////////////////////////////
#pragma mark Private methods
//////////////////////////////////////////////////////////////

- (void)actionDelete
{
    if (self.deleteBlock)
    {
        self.deleteBlock(self);
    }
}

#pragma mark - Touch Event Delegate

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    self.highLighted = YES;
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    self.highLighted = NO;
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    self.highLighted = NO;
}

@end
