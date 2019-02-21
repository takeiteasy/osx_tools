//
//  kbd.m
//  kbd
//
//  Created by Rory B. Bellows on 20/02/2019.
//  Copyright © 2019 Rory B. Bellows. All rights reserved.
//
// clang kbd.m -framework Foundation -framework Carbon

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#import <Foundation/Foundation.h>
#import <Carbon/Carbon.h>

static CFMachPortRef event_tap = nil;
static CFRunLoopSourceRef loop = nil;

#define MOD_CHECK \
  X(kCGEventFlagMaskAlphaShift, @"CAPS+") \
  X(kCGEventFlagMaskShift, @"SHIFT+") \
  X(kCGEventFlagMaskControl, @"CTRL+") \
  X(kCGEventFlagMaskAlternate, @"ALT+") \
  X(kCGEventFlagMaskCommand, @"CMD+")

NSMutableString* event_key_str(CGEventRef event, CGEventType type) {
  CGEventFlags flags = CGEventGetFlags(event);
  NSMutableString* mod_str = [NSMutableString string];
  
#define X(x, y) \
  if (YES == !!(flags & x)) \
    [mod_str appendString:y];
  MOD_CHECK
#undef X
  
  if (mod_str && [mod_str length])
    [mod_str replaceCharactersInRange:NSMakeRange([mod_str length] - 1, 1) withString:@" "];
  
  CGKeyCode keyCode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
  
  CFDataRef keylayoutData = (CFDataRef)TISGetInputSourceProperty(TISCopyCurrentKeyboardInputSource(), kTISPropertyUnicodeKeyLayoutData);
  if (!keylayoutData)
    return nil;
  
  const UCKeyboardLayout* keyboardLayout = (const UCKeyboardLayout*)CFDataGetBytePtr(keylayoutData);
  if (!keyboardLayout)
    return nil;
  
  UInt16 keyAction = (type = kCGEventKeyDown ? kUCKeyActionDown : kUCKeyActionUp);
  UInt32 modifierState = 0, deadKeyState = 0;
  UniCharCount maxStringLength = 255, actualStringLength = 0;
  UniChar unicodeString[maxStringLength];
  memset(unicodeString, 0x0, sizeof(unicodeString));
  OSStatus status = UCKeyTranslate(keyboardLayout, keyCode, keyAction, modifierState, LMGetKbdType(), 0, &deadKeyState, maxStringLength, &actualStringLength, unicodeString);
  if (status != noErr || !actualStringLength)
    return nil;
  
  [mod_str appendString:[[NSString stringWithCharacters:unicodeString length:(NSUInteger)actualStringLength] uppercaseString]];
  return mod_str;
}

CGEventRef event_cb(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* ref) {
  bool down = true;
  NSMutableString* ret = nil;
  switch (type) {
    case kCGEventKeyUp:
      down = false;
    case kCGEventKeyDown:
      ret = event_key_str(event, type);
      if (!ret)
        break;
      [ret appendFormat:@" %@", down ? @"DOWN" : @"UP"];
      // DO SOMETHING
      break;
    case kCGEventTapDisabledByUserInput:
    case kCGEventTapDisabledByTimeout:
      CGEventTapEnable(event_tap, true);
    default:
      break;
  }
  return event;
}

CFMachPortRef create_tap() {
  return CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp), event_cb, NULL);
}

void _atexit() {
  CFRelease(event_tap);
  CFRelease(loop);
}

void _signal(int sig) {
  switch(sig) {
    case SIGHUP:
      CGEventTapEnable(event_tap, true);
      break;
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
    case SIGABRT:
      _atexit();
      break;
    default:
      break;
  }
}

int main(int argc, const char* argv[]) {
  if (geteuid()) {
    NSLog(@"ERROR: Run as root");
    return EXIT_FAILURE;
  }
  
  atexit(_atexit);
  signal(SIGINT, _signal);
  
  @autoreleasepool {
    event_tap = create_tap();
    if (!event_tap) {
      printf("Waiting for permission");
      while (!(event_tap = create_tap())) {
        printf(".");
        fflush(stdout);
        usleep(1000000);
      }
      printf("SUCCESS!\n");
    }
    loop = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), loop, kCFRunLoopCommonModes);
    CGEventTapEnable(event_tap, true);
    CFRunLoopRun();
  }
  return EXIT_SUCCESS;
}
