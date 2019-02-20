//
//  nosleep.m
//  nosleep
//
//  Created by Rory B. Bellows on 09/02/2019.
//  Copyright © 2019 Rory B. Bellows. All rights reserved.
//
// clang nosleep.m -framework IOKit -framework Foundation -framework Cocoa

#import <Cocoa/Cocoa.h>
#import <Availability.h>
#import <IOKit/pwr_mgt/IOPMLib.h>

static CFStringRef reasonForActivity = CFSTR("DON'T SLEEP!");
static IOPMAssertionID assertionID;

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong, nonatomic) NSStatusItem* statusItem;
@end

@implementation AppDelegate : NSObject
- (id)init {
  if (self = [super init]) {
    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
    [nc addObserver:self
           selector:@selector(onExit:)
               name:NSApplicationWillTerminateNotification
             object:nil];
    
    _statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    
    float scale = 1.f;
    if ([[NSScreen mainScreen] respondsToSelector:@selector(backingScaleFactor)]) {
      NSArray* screens = [NSScreen screens];
      NSUInteger screenCount = screens.count;
      for (int i = 0; i < screenCount; i++) {
        float s = [screens[i] backingScaleFactor];
        if (s > scale)
          scale = s;
      }
    }
    
    [_statusItem button].title = @"";
    if (scale > 1.f) {
      [_statusItem button].image = [[NSImage alloc] initWithContentsOfFile:@"/Users/rory/test@2x.png"];
      [_statusItem button].alternateImage = [[NSImage alloc] initWithContentsOfFile:@"/Users/rory/test-alt@2x.png"];
    } else {
      [_statusItem button].image = [[NSImage alloc] initWithContentsOfFile:@"/Users/rory/test.png"];
      [_statusItem button].alternateImage = [[NSImage alloc] initWithContentsOfFile:@"/Users/rory/test-alt.png"];
    }
    
#ifdef __MAC_OS_X_VERSION_MAX_ALLOWED
#if __MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4
    _statusItem.highlightMode = YES;
#endif
#endif
    
    NSMenu *menu = [[NSMenu alloc] init];
    
//    [menu addItemWithTitle:@"This is a test!" action:@selector(doSomething:) keyEquivalent:@""];
//    [menu addItem:[NSMenuItem separatorItem]]; // A thin grey line
    
    [menu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@""];
    _statusItem.menu = menu;
    
    if (IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn, reasonForActivity, &assertionID) != kIOReturnSuccess) {
      NSLog(@"ERROR! Failed to prevent sleep!");
      [NSApp terminate:nil];
    }
  }
  return self;
}

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
  [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
}

- (void)onExit:(id)sender {
  IOPMAssertionRelease(assertionID);
}

//- (void)doSomething:(id)sender {
//  NSLog(@"Done something!");
//}
@end

int main(int argc, const char * argv[]) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    AppDelegate* app_del = [[AppDelegate alloc] init];
    [NSApp setDelegate:app_del];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
  }
  return 0;
}
