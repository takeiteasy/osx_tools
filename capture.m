//
//  main.m
//  cature
//
//  Created by Rory B. Bellows on 23/12/2017.
//  Copyright © 2017 Rory B. Bellows. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#import <AppKit/AppKit.h>
#include <time.h>

@interface capture_del_t : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property AVCaptureSession* session;
-(void)  captureOutput:(AVCaptureOutput*)output
 didOutputSampleBuffer:(CMSampleBufferRef)buffer
        fromConnection:(AVCaptureConnection*)connection;
@end

@interface capture_del_t() {
  CVImageBufferRef head;
  CFRunLoopRef loop;
  int count;
}
-(void)output;
@end

@implementation capture_del_t
@synthesize session;
-(id)init {
  self  = [super init];
  loop  = CFRunLoopGetCurrent();
  head  = nil;
  count = 0;
  return self;
}

-(void)  captureOutput:(AVCaptureOutput*)output
 didOutputSampleBuffer:(CMSampleBufferRef)buffer
        fromConnection:(AVCaptureConnection*)connection {
  CVImageBufferRef prev, frame = CMSampleBufferGetImageBuffer(buffer);
  CVBufferRetain(frame);
  
  @synchronized (self) {
    prev = head;
    head = frame;
    count++;
  }
  
  CVBufferRelease(prev);
  if (count == 6) {
    [self output];
    [self.session stopRunning];
    CFRunLoopStop(loop);
  }
}

-(void)output {
  @synchronized (self) {
    char out[512];
    sprintf(out, "%lu.png", (unsigned long)time(NULL));
    
    CIImage* cii = [CIImage imageWithCVImageBuffer:head];
    NSBitmapImageRep* rep = [[NSBitmapImageRep alloc] initWithCIImage:cii];
    NSData* png = [rep representationUsingType:NSPNGFileType
                                    properties:nil];
    [png writeToFile:[NSString stringWithUTF8String:out]
          atomically:NO];
  }
}
@end

int main(int argc, const char * argv[]) {
  @autoreleasepool {
    NSError* err = nil;
    AVCaptureDevice* device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    AVCaptureDeviceInput* input = [AVCaptureDeviceInput deviceInputWithDevice:device
                                                                        error:&err];
    AVCaptureVideoDataOutput* output = [[AVCaptureVideoDataOutput alloc] init];
    capture_del_t* capture = [[capture_del_t alloc] init];
    [output setSampleBufferDelegate:capture
                              queue:dispatch_get_main_queue()];
    
    AVCaptureSession* session = [[AVCaptureSession alloc] init];
    [session addInput: input];
    [session addOutput: output];
    capture.session = session;
    [session startRunning];
    
    CFRunLoopRun();
  }
  return 0;
}
