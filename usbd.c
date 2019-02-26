//
//  usbd.c
//  usbd
//
//  Created by Rory B. Bellows on 19/02/2019.
//  Copyright © 2019 Rory B. Bellows. All rights reserved.
//
// clang usbd.c -framework IOKit -framework Foundation

#include <stdio.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/usb/IOUSBLib.h>

static io_iterator_t add_iter, remove_iter;

void isAttached(void *refcon, io_iterator_t iterator) {
  io_service_t usbDevice;
  while((usbDevice = IOIteratorNext(iterator))) {
    io_name_t name;
    IORegistryEntryGetName(usbDevice, name);
    printf("\tName:\t\t%s\n", (char *)name);
    
    CFNumberRef idProduct = (CFNumberRef)IORegistryEntrySearchCFProperty(usbDevice, kIOServicePlane, CFSTR("idProduct"), kCFAllocatorDefault, 0);
    uint16_t PID;
    CFNumberGetValue(idProduct, kCFNumberSInt16Type, (void *)&PID);
    printf("\tidProduct:\t0x%x\n", PID);
    
    IOObjectRelease(usbDevice);
    CFRelease(idProduct);
  }
}

static void Handle_UsbDetectionCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef) {
  printf("test\n");
}

static void Handle_UsbRemoveCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef) {
  printf("test2\n");
}

int main(int argc, const char * argv[]) {
  CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOUSBDeviceClassName);
  CFRetain(matchingDict);
  
  IONotificationPortRef add_notify_port = IONotificationPortCreate(kIOMasterPortDefault), remove_notify_port = IONotificationPortCreate(kIOMasterPortDefault);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(add_notify_port), kCFRunLoopDefaultMode);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(remove_notify_port), kCFRunLoopDefaultMode);
  IOHIDManagerRef hid_manager = IOHIDManagerCreate(kCFAllocatorDefault, 0L);
  IOReturn ret = IOHIDManagerOpen(hid_manager, 0L);
  if (ret != kIOReturnSuccess) {
    fprintf(stderr, "IOHIDManagerOpen() failed\n");
    return EXIT_FAILURE;
  }
  IOHIDManagerSetDeviceMatching(hid_manager, NULL);
  IOHIDManagerScheduleWithRunLoop(hid_manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
  IOHIDManagerRegisterDeviceMatchingCallback(hid_manager, Handle_UsbDetectionCallback, NULL);
  IOHIDManagerRegisterDeviceRemovalCallback(hid_manager, Handle_UsbRemoveCallback, NULL);
  IOServiceAddMatchingNotification(add_notify_port, kIOFirstMatchNotification, matchingDict, isAttached, NULL, &add_iter);
  IOServiceAddMatchingNotification(remove_notify_port, kIOTerminatedNotification, matchingDict, isAttached, NULL, &remove_iter);
  isAttached(NULL, add_iter);
  isAttached(NULL, remove_iter);

  CFRunLoopRun();
  return 0;
}
