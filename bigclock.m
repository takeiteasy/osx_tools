//
//  bigclock.m
//  bigclock
//
//  Created by Rory B. Bellows on 25/02/2019.
//  Copyright © 2019 Rory B. Bellows. All rights reserved.
//
// clang bigclock.m -framework Cocoa

#import <Cocoa/Cocoa.h>

typedef enum {
  fadein,
  fadeout,
  nothing
} fade_state;
static fade_state state = nothing;
static double opacity_off = 0.;

@interface AppView : NSView {}
@end

@implementation AppView
- (id)initWithFrame:(NSRect)frame {
  if (self = [super initWithFrame:frame]) {
    [self addTrackingRect:[self visibleRect]
                    owner:self
                 userData:nil
             assumeInside:NO];
  }
  return self;
}

- (void)drawRect:(NSRect)frame {
  NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:frame
                                                       xRadius:6.
                                                       yRadius:6.];
  [[NSColor colorWithRed:0
                   green:0
                    blue:0
                   alpha:.25 + opacity_off] set];
  [path fill];
}

- (void)mouseEntered:(NSEvent*)event {
  state = fadein;
}

- (void)mouseExited:(NSEvent*)event {
  state = fadeout;
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
  NSWindow* window;
  AppView* view;
  NSTextField* label;
  NSTimer* timer;
  NSTimer* fade_timer;
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
    timer = [NSTimer scheduledTimerWithTimeInterval:1.
                                             target:self
                                           selector:@selector(update)
                                           userInfo:nil
                                            repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:timer
                                 forMode:NSModalPanelRunLoopMode];
    fade_timer = [NSTimer scheduledTimerWithTimeInterval:(1. / 60.)
                                                  target:self
                                                selector:@selector(fade_update)
                                                userInfo:nil
                                                 repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:fade_timer
                                 forMode:NSModalPanelRunLoopMode];
  }
  return self;
}

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
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
  [window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces];
  
  [label setStringValue:@""];
  [label setBezeled:NO];
  [label setDrawsBackground:NO];
  [label setEditable:NO];
  [label setSelectable:NO];
  [label setAlignment:NSTextAlignmentCenter];
  [label setFont:[NSFont systemFontOfSize:72.]];
  [label setTextColor:[[NSColor whiteColor] colorWithAlphaComponent:.5]];
  [[label cell] setBackgroundStyle:NSBackgroundStyleRaised];
  
  [window setContentView:view];
  [view addSubview:label];
  [self update];
}

- (void)update {
  NSDateFormatter* fmt = [[NSDateFormatter alloc] init];
  [fmt setDateFormat:@"hh:mm:ss"];
  [label setStringValue:[fmt stringFromDate:[NSDate date]]];
  [view setNeedsDisplay:YES];
}

- (void)update_fade:(NSTimeInterval)v {
  opacity_off += (v / 10.);
  [label setTextColor:[[NSColor whiteColor] colorWithAlphaComponent:.5 + opacity_off]];
  [view setNeedsDisplay:YES];
}

- (void)fade_update {
  switch (state) {
    case fadein:
      if (opacity_off < .5)
        [self update_fade:[[timer fireDate] timeIntervalSinceDate:[NSDate date]]];
      else {
        opacity_off = .5;
        state = nothing;
      }
      break;
    case fadeout:
      if (opacity_off > 0)
        [self update_fade:-([[timer fireDate] timeIntervalSinceDate:[NSDate date]])];
      else {
        opacity_off = 0;
        state = nothing;
      }
      break;
    case nothing:
    default:
      break;
  }
}
@end

int main(int argc, const char* argv[]) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
    [NSApp setDelegate:[AppDelegate new]];
    [NSApp run];
  }
  return 0;
}
