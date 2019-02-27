//
//  largetype.m
//  largetype
//
//  Created by Rory B. Bellows on 26/02/2019.
//  Copyright © 2019 Rory B. Bellows. All rights reserved.
//
// clang largetype.m -framework Cocoa

#import <Cocoa/Cocoa.h>

static NSString* str = nil;

#define PADDING 10
#define TIMEOUT 4.

@interface AppView : NSView {
  double a;
}
@end

@implementation AppView
- (id)initWithFrame:(NSRect)frame {
  if (self = [super initWithFrame:frame]) {}
  return self;
}

- (void)drawRect:(NSRect)frame {
  NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:frame
                                                       xRadius:6.0
                                                       yRadius:6.0];
  [[NSColor colorWithRed:0
                   green:0
                    blue:0
                   alpha:a] set];
  [path fill];
}

- (void)setAlpha:(double)_a {
  a = _a;
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
  NSWindow* window;
  AppView* view;
  NSTextField* label;
  NSTimer* timer;
  NSDate* start;
}
@end

@implementation AppDelegate : NSObject
- (id)init {
  if (self = [super init]) {
    window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 0, 0)
                                         styleMask:NSWindowStyleMaskBorderless
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
    label = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
    view = [[AppView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
    timer = [NSTimer scheduledTimerWithTimeInterval:(1. / 60.)
                                             target:self
                                           selector:@selector(update)
                                           userInfo:nil
                                            repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:timer
                                 forMode:NSModalPanelRunLoopMode];
    start = [NSDate date];
  }
  return self;
}

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
  [label setStringValue:str];
  [label setBezeled:NO];
  [label setDrawsBackground:NO];
  [label setEditable:NO];
  [label setSelectable:NO];
  [label setAlignment:NSTextAlignmentCenter];
  [label setFont:[NSFont systemFontOfSize:72.0]];
  [label setTextColor:[NSColor whiteColor]];
  [[label cell] setBackgroundStyle:NSBackgroundStyleRaised];
  [label sizeToFit];
  
  [window setTitle:NSProcessInfo.processInfo.processName];
  [window setOpaque:NO];
  [window setExcludedFromWindowsMenu:NO];
  [window setBackgroundColor:[NSColor clearColor]];
  [window setIgnoresMouseEvents:YES];
  [window makeKeyAndOrderFront:self];
  [window setLevel:NSFloatingWindowLevel];
  [window setCanHide:NO];
  
  [window setFrame:NSMakeRect(([[NSScreen mainScreen] visibleFrame].origin.x + [[NSScreen mainScreen] visibleFrame].size.width / 2) - ([label frame].size.width / 2),
                              ([[NSScreen mainScreen] visibleFrame].origin.y + [[NSScreen mainScreen] visibleFrame].size.height / 2) - ([label frame].size.height / 2),
                              [label frame].size.width + PADDING,
                              [label frame].size.height + PADDING)
           display:YES];
  [view setFrame:NSMakeRect(0, 0,
                            [window frame].size.width,
                            [window frame].size.height)];
  [label setFrame:NSMakeRect(0, 0,
                             [window frame].size.width,
                             [window frame].size.height)];
  [window setContentView:view];
  [view addSubview:label];
}

- (void)update {
  NSTimeInterval elapsed = [[NSDate date] timeIntervalSinceDate:start];
  if (elapsed >= TIMEOUT)
    [NSApp terminate:nil];
  double a = 1. - (elapsed / TIMEOUT);
  [view setAlpha:a];
  [label setTextColor:[[NSColor whiteColor] colorWithAlphaComponent:a]];
  [view setNeedsDisplay:YES];
}
@end

int main(int argc, char** argv) {
  @autoreleasepool {
    if (isatty(fileno(stdin)))
      return EXIT_FAILURE;
    
    NSMutableString* pipe = [[NSMutableString alloc] init];
    char line[LINE_MAX];
    while (fgets(line, LINE_MAX, stdin) != NULL)
      [pipe appendString:@(line)];
    
    if (![pipe length])
      return EXIT_FAILURE;
    if ([pipe characterAtIndex:[pipe length] - 1] == '\n')
      [pipe deleteCharactersInRange:NSMakeRange([pipe length] - 1, 1)];
    
    str = [[NSString alloc] initWithString:pipe];
    if (!str)
      return EXIT_FAILURE;
    
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
    [NSApp setDelegate:[AppDelegate new]];
    [NSApp run];
  }
  return EXIT_SUCCESS;
}
