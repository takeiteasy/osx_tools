//
//  main.m
//  clock
//
//  Created by Rory B. Bellows on 25/02/2019.
//  Copyright © 2019 Rory B. Bellows. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface AppView : NSView {}
@end

@implementation AppView
- (id)initWithFrame:(NSRect)frame {
  if (self = [super initWithFrame:frame]) {}
  return self;
}

- (void)drawRect:(NSRect)frame {
  NSBezierPath *path = [NSBezierPath bezierPathWithRoundedRect:frame xRadius:6.0 yRadius:6.0];
  [[NSColor colorWithRed:0 green:0 blue:0 alpha:.75] set];
  [path fill];
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
  NSWindow* window;
  AppView* view;
  NSTextField* label;
  NSTimer* timer;
}
@end

@implementation AppDelegate : NSObject
- (id)init {
  if (self = [super init]) {
    window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 310, 95)
                                       styleMask:NSWindowStyleMaskBorderless
                                         backing:NSBackingStoreBuffered
                                           defer:NO];
    label = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 310, 92)];
    view = [[AppView alloc] initWithFrame:NSMakeRect(0, 0, 200, 200)];
    timer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 60.0)
                                             target:self
                                           selector:@selector(update)
                                           userInfo:nil
                                            repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:timer
                                 forMode:NSEventTrackingRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:timer
                                 forMode:NSModalPanelRunLoopMode];
  }
  return self;
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
  [window setTitle:NSProcessInfo.processInfo.processName];
  [window setFrameOrigin:NSMakePoint([[NSScreen mainScreen] visibleFrame].origin.x + [[NSScreen mainScreen] visibleFrame].size.width - [window frame].size.width - 20,
                                     [[NSScreen mainScreen] visibleFrame].origin.y + [[NSScreen mainScreen] visibleFrame].size.height - [window frame].size.height - 30)];
  [window setOpaque:NO];
  [window setExcludedFromWindowsMenu:NO];
  [window setBackgroundColor:[NSColor clearColor]];
  [window setIgnoresMouseEvents:YES];
  [window makeKeyAndOrderFront:self];
  [window setLevel:NSFloatingWindowLevel];
  [window setCanHide:NO];
  
  [label setStringValue:@""];
  [label setBezeled:NO];
  [label setDrawsBackground:NO];
  [label setEditable:NO];
  [label setSelectable:NO];
  [label setAlignment:NSTextAlignmentCenter];
  [label setFont:[NSFont systemFontOfSize:72.0]];
  [label setTextColor:[NSColor whiteColor]];
  [[label cell] setBackgroundStyle:NSBackgroundStyleRaised];
  
  [window setContentView:view];
  [view addSubview:label];
}

- (void)update {
  NSDateFormatter* fmt = [[NSDateFormatter alloc] init];
  [fmt setDateFormat:@"hh:mm:ss"];
  [label setStringValue:[fmt stringFromDate:[NSDate date]]];
  [view setNeedsDisplay:YES];
}
@end

int main(int argc, const char * argv[]) {
  @autoreleasepool {
    NSApplication* app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
    AppDelegate* appDelegate = [AppDelegate new];
    [app setDelegate:appDelegate];
    [app run];
  }
  return 0;
}
