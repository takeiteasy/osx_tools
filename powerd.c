//
//  main.c
//  powerd
//
//  Created by Rory B. Bellows on 16/02/2019.
//  Copyright © 2019 Rory B. Bellows. All rights reserved.
//
// clang powerd.c -framework IOKit -framework Foundation

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <mach/mach_port.h>
#include <mach/mach_interface.h>
#include <mach/mach_init.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>

#if DEBUG
#define LOG(MSG, ...) fprintf(stderr, "[DEBUG] from %s in %s at %d -- " MSG "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define LOG(MSG, ...)
#endif
#define GET_ERRNO() (errno == 0 ? "None" : strerror(errno))
#define LOG_ERR(MSG, ...) fprintf(stderr, "[ERROR] from %s in %s at %d (errno: %s) " MSG "\n", __FILE__, __FUNCTION__, __LINE__, GET_ERRNO(), ##__VA_ARGS__)

#define SAFE_RELEASE(x) \
if ((x)) \
  CFRelease((x));

#define CONFIG_DEF \
  CFG(sleep, NULL) \
  CFG(wakeup, NULL) \
  CFG(cantsleep, NULL) \
  CFG(cansleep, NULL) \
  CFG(idle, NULL) \
  CFG(resume, NULL) \
  CFG(timeout, "1000") \
  CFG(powerac, NULL) \
  CFG(powerbattery, NULL) \
  CFG(batterylow, NULL) \
  CFG(batterycritical, NULL) \
  CFG(displaydim, NULL) \
  CFG(displaysleep, NULL) \
  CFG(displaywakeup, NULL) \
  CFG(atstart, NULL) \
  CFG(atexit, NULL)

typedef struct {
#define CFG(n, default) char *n;
  CONFIG_DEF
#undef CFG
} config_t;

static config_t config = {
#define CFG(n, default) default,
  CONFIG_DEF
#undef CFG
};

void run_command(const char* event, const char* cmd) {
  printf("%s event triggered: executing %s ...\n", event, (cmd ? cmd : "Nothing"));
  if (!cmd || !event)
    return;
  
  FILE* fd = popen(cmd, "r");
  if (!fd)
    return;
  
  char buffer[256];
  size_t chread;
  size_t comalloc = 256;
  size_t comlen = 0;
  char  *comout = malloc(comalloc);
  
  while ((chread = fread(buffer, 1, sizeof(buffer), fd)) != 0) {
    if (comlen + chread >= comalloc) {
      comalloc *= 2;
      comout = realloc(comout, comalloc);
    }
    memmove(comout + comlen, buffer, chread);
    comlen += chread;
  }
  
  fwrite(comout, 1, comlen, stdout);
  free(comout);
  pclose(fd);
}
#define RUN(n) run_command(#n, config.n)

static long int get_idle_time() {
  mach_port_t master_port = 0;
  io_iterator_t iter = 0;
  io_registry_entry_t cur_obj = 0;
  CFMutableDictionaryRef properties = NULL;
  CFTypeRef obj = NULL;
  CFTypeID type = 0;
  uint64_t idle_time = -1;
  
  if (IOMasterPort(MACH_PORT_NULL, &master_port) != KERN_SUCCESS) {
    LOG_ERR("IOMasterPort() failed");
    goto END;
  }
  
  IOServiceGetMatchingServices(master_port, IOServiceMatching(kIOHIDSystemClass), &iter);
  if (iter == 0) {
    LOG_ERR("IOServiceGetMatchingServices() failed");
    goto END;
  }
  
  cur_obj = IOIteratorNext(iter);
  if (cur_obj == 0) {
    LOG_ERR("IOIteratorNext() failed");
    goto END;
  }
  
  if (IORegistryEntryCreateCFProperties(cur_obj, &properties, kCFAllocatorDefault, 0) != KERN_SUCCESS || !properties) {
    LOG_ERR("IORegistryEntryCreateCFProperties() failed");
    goto END;
  }
  
  obj = CFDictionaryGetValue(properties, CFSTR(kIOHIDIdleTimeKey));
  CFRetain(obj);
  type = CFGetTypeID(obj);
  if (type == CFDataGetTypeID())
    CFDataGetBytes ((CFDataRef)obj, CFRangeMake(0, sizeof(idle_time)), (UInt8*)&idle_time);
  else if (type == CFNumberGetTypeID())
    CFNumberGetValue ((CFNumberRef)obj, kCFNumberSInt64Type, &idle_time);
  else {
    LOG_ERR("CFGetTypeID() failed");
    goto END;
  }
  idle_time /= 1000000000l * 0.1;
  
END:
  if (master_port)
    mach_port_deallocate(mach_task_self(), master_port);
  SAFE_RELEASE(obj);
  if (cur_obj)
    IOObjectRelease(cur_obj);
  if (iter)
    IOObjectRelease(iter);
  SAFE_RELEASE((CFTypeRef)properties);
  return idle_time;
}

static CFRunLoopTimerRef setup_timer_loop(long int timeout, CFRunLoopTimerRef timer, CFRunLoopTimerCallBack callback) {
  if (timeout) {
    if (timer)
      CFRunLoopTimerSetNextFireDate(timer, CFAbsoluteTimeGetCurrent() + timeout * 0.1);
    else {
      timer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + timeout * 0.1, kCFAbsoluteTimeIntervalSince1904, 0, 0, callback, NULL);
      CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopDefaultMode);
    }
  } else {
    if (timer) {
      CFRunLoopTimerInvalidate(timer);
      CFRelease(timer);
      timer = NULL;
    }
  }
  return timer;
}

static int idle_resume = 0;
static long int idle_timeout = 1000;

static void idle_cb(CFRunLoopTimerRef timer, void* info) {
  LOG("idle for %ld * 0.1 seconds (timeout: %ld * 0.1 seconds)", get_idle_time(), idle_timeout);
  RUN(idle);
  idle_resume = 1;
}

static void setup_idle_timer() {
  static CFRunLoopTimerRef idle_timer = NULL;
  if (config.timeout) {
    idle_timeout = atoi(config.timeout);
    if (idle_timeout <= 0)
      idle_timeout = 1000;
  }
  idle_timer = setup_timer_loop(idle_timeout, idle_timer, idle_cb);
}

typedef enum POWER_SRC {
  UNKNOWN = -1,
  BATTERY =  0,
  AC      =  1
} POWER_SRC;

void power_cb(void* root_port, io_service_t y, natural_t msg_type, void* msg_args) {
  LOG("power_cb: msg_type %08lx, msg_args %08lx", (long unsigned int)msg_type, (long  unsigned int)msg_args);
  
  switch (msg_type) {
    case kIOMessageCanSystemSleep:
      RUN(cansleep);
      IOAllowPowerChange(*(io_connect_t*)root_port, (long)msg_args);
      break;
    case kIOMessageSystemWillSleep:
      RUN(sleep);
      IOAllowPowerChange(*(io_connect_t*)root_port, (long)msg_args);
      break;
    case kIOMessageSystemWillNotSleep:
      RUN(cantsleep);
      break;
    case kIOMessageSystemHasPoweredOn:
      RUN(wakeup);
      setup_idle_timer();
      break;
    default:
      break;
  }
}

POWER_SRC get_power_src() {
  POWER_SRC res = UNKNOWN;
  CFTypeRef info = NULL, src = NULL;
  CFArrayRef power_srcs = NULL;
  CFDictionaryRef desc = NULL;
  CFStringRef state = NULL;
  
#define CHECK(x) \
  if (!(x)) \
    goto END;
  CHECK(info = IOPSCopyPowerSourcesInfo());
  CHECK(power_srcs = IOPSCopyPowerSourcesList(info));
  CHECK(CFArrayGetCount(power_srcs));
  CHECK(src = CFArrayGetValueAtIndex(power_srcs, 0));
  CHECK(desc = IOPSGetPowerSourceDescription(info, src));
  CHECK(state = CFDictionaryGetValue(desc, CFSTR(kIOPSPowerSourceStateKey)));
#undef CHECK
  
  res = (CFStringCompare(state, CFSTR(kIOPSACPowerValue), 0) == kCFCompareEqualTo) ? AC : BATTERY;
  
END:
  SAFE_RELEASE(info);
  SAFE_RELEASE(power_srcs);
//  SAFE_RELEASE(src);
//  SAFE_RELEASE(desc);
  SAFE_RELEASE(state);
  return res;
}

void limited_power_src_cb(void* ctx) {
  static IOPSLowBatteryWarningLevel last_level = kIOPSLowBatteryWarningNone;
  
  IOPSLowBatteryWarningLevel level = IOPSGetBatteryWarningLevel();
  if (level == kIOPSLowBatteryWarningNone || (last_level == kIOPSLowBatteryWarningEarly && level == last_level))
    return;
  
  switch (level) {
    case kIOPSLowBatteryWarningEarly:
      RUN(batterylow);
      break;
    case kIOPSLowBatteryWarningFinal:
      RUN(batterycritical);
      break;
    case kIOPSLowBatteryWarningNone:
    default:
      break;
  }
  last_level = level;
}

void power_src_cb(void* ctx) {
  static POWER_SRC last_power_src = UNKNOWN;
  
  POWER_SRC power_src = get_power_src();
  if (power_src == UNKNOWN || power_src == last_power_src)
    return;
  
  switch (power_src) {
    case AC:
      RUN(powerac);
      break;
    case BATTERY: {
      RUN(powerbattery);
      break;
    }
    case UNKNOWN:
    default:
      LOG_ERR("power_src_cb() failed: Can't get power source");
      exit(EXIT_FAILURE);
  }
  last_power_src = power_src;
}

void display_cb(void* ctx, io_service_t y, natural_t msg_type, void* msg_args) {
  static enum {
    ON,
    DIMMED,
    OFF
  } state = ON;
  
  LOG("display_cb: msg_type %08lx, msg_args %08lx", (long unsigned int)msg_type, (long  unsigned int)msg_args);
  
  switch (msg_type) {
    case kIOMessageDeviceWillPowerOff:
      switch(state++) {
        case DIMMED:
          RUN(displaydim);
          break;
        case OFF:
          RUN(displaysleep);
          break;
        case ON:
        default:
          break;
      }
      break;
    case kIOMessageDeviceHasPoweredOn:
      if (state == DIMMED)
        RUN(displaywakeup);
      state = ON;
      break;
    default:
      break;
  }
}

static void hid_cb (void* ctx, IOReturn res, void* sender, IOHIDValueRef value) {
  if (idle_resume) {
    RUN(resume);
    idle_resume = 0;
  }
  setup_idle_timer();
}


static CFMutableDictionaryRef createDeviceMatchingDictionary(UInt32 usage_page, UInt32 usage) {
  CFMutableDictionaryRef result = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
  if (!result) {
    LOG_ERR("CFDictionaryCreateMutable() failed");
    exit(EXIT_FAILURE);
  }
  
  CFNumberRef pageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_page);
  if (!pageCFNumberRef) {
    LOG_ERR("CFNumberCreate() failed");
    exit(EXIT_FAILURE);
  }
  
  CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsagePageKey), pageCFNumberRef);
  CFRelease(pageCFNumberRef);
  CFNumberRef usageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
  if (!usageCFNumberRef) {
    LOG_ERR("CFNumberCreate() failed");
    exit(EXIT_FAILURE);
  }
  
  CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsageKey), usageCFNumberRef);
  CFRelease(usageCFNumberRef);
  return result;
}


static CFArrayRef createGenericDesktopMatchingDictionaries(void) {
  CFMutableArrayRef matchingCFArrayRef = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
  if (!matchingCFArrayRef) {
    LOG_ERR("CFArrayCreateMutable() failed");
    exit(EXIT_FAILURE);
  }
  
  CFDictionaryRef matchingCFDictRef = createDeviceMatchingDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse);
  if (!matchingCFDictRef) {
    LOG_ERR("createDeviceMatchingDictionary() failed");
    exit(EXIT_FAILURE);
  }
  CFArrayAppendValue(matchingCFArrayRef, matchingCFDictRef);
  CFRelease(matchingCFDictRef);
  
  matchingCFDictRef = createDeviceMatchingDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
  if (!matchingCFDictRef) {
    LOG_ERR("createDeviceMatchingDictionary() failed");
    exit(EXIT_FAILURE);
  }
  CFArrayAppendValue(matchingCFArrayRef, matchingCFDictRef);
  CFRelease(matchingCFDictRef);
  
  return matchingCFArrayRef;
}

void parse_config() {
  char home_buf[BUFSIZ];
  sprintf(home_buf, "%s/%s", getenv("HOME"), ".powerd");
  if (access(home_buf, F_OK ) == -1) {
    LOG_ERR("ERROR: %s doesn't exist!", home_buf);
    exit(EXIT_FAILURE);
  }
  
  FILE* fp = fopen(home_buf, "r");
  if (!fp) {
    LOG_ERR("ERROR: Can't open %s!", home_buf);
    exit(EXIT_FAILURE);
  }
  
#define DELIM "="
  char *line = NULL, *token;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, fp)) != -1) {
    if (line[read - 1] == '\n') {
      line[read - 1] = '\0';
      read--;
    }
    if (!read || line[0] == '#')
    continue;
    
    token = strtok(line, DELIM);
#define CFG(n, default) \
if (token && !strcmp(token, #n)) { \
token = strtok(NULL, DELIM); \
if (token) \
config.n = strdup(token); \
}
    CONFIG_DEF
#undef CFG
  }
  fclose(fp);
}

void signal_cb(int sig) {
  RUN(atexit);
#define CFG(n, default) if (config.n) free(config.n);
  CONFIG_DEF
#undef CFG
  
  if (sig == SIGHUP)
    setup_idle_timer();
  else
    exit(EXIT_SUCCESS);
}

int main(int argc, const char * argv[]) {
  static io_connect_t pw_root_port;
  io_connect_t dp_root_port;
  IONotificationPortRef pw_notifier_port, dp_notifier_port;
  io_object_t pw_notifier, dp_notifier;
  CFRunLoopSourceRef lp_src, pw_src;
  IOHIDManagerRef hid_manager;
  
  
  parse_config();
  RUN(atstart);
  signal(SIGHUP, signal_cb);
  signal(SIGINT, signal_cb);
  signal(SIGTERM, signal_cb);
  
  if (!(pw_root_port = IORegisterForSystemPower(&pw_root_port, &pw_notifier_port, power_cb, &pw_notifier))) {
    LOG_ERR("IORegisterForSystemPower() failed");
    return EXIT_FAILURE;
  }
  
  if (!(lp_src = IOPSCreateLimitedPowerNotification(limited_power_src_cb, NULL))) {
    LOG_ERR("IOPSCreateLimitedPowerNotification() failed");
    return EXIT_FAILURE;
  }
  
  if (!(pw_src = IOPSNotificationCreateRunLoopSource(power_src_cb, NULL))) {
    LOG_ERR("IOPSNotificationCreateRunLoopSource() failed");
    return EXIT_FAILURE;
  }
  
  if (!(dp_root_port = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceNameMatching("IODisplayWrangler")))) {
    LOG_ERR("IOServiceGetMatchingService() failed");
    return EXIT_FAILURE;
  }
  if (!(dp_notifier_port = IONotificationPortCreate(kIOMasterPortDefault))) {
    LOG_ERR("IONotificationPortCreate() failed");
    return EXIT_FAILURE;
  }
  if (IOServiceAddInterestNotification(dp_notifier_port, dp_root_port, kIOGeneralInterest, display_cb, NULL, &dp_notifier) != kIOReturnSuccess) {
    LOG_ERR("IOServiceAddInterestNotification() failed");
    return EXIT_FAILURE;
  }
  
  if (!(hid_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone))) {
    LOG_ERR("IOHIDManagerCreate() failed");
    return EXIT_FAILURE;
  }
  if (IOHIDManagerOpen(hid_manager, kIOHIDOptionsTypeNone) != kIOReturnSuccess) {
    LOG_ERR("IOHIDManagerOpen() failed");
    return EXIT_FAILURE;
  }
  
  setup_idle_timer();
  IOHIDManagerScheduleWithRunLoop (hid_manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
  IOHIDManagerSetDeviceMatchingMultiple (hid_manager, createGenericDesktopMatchingDictionaries());
  IOHIDManagerRegisterInputValueCallback (hid_manager, hid_cb, (void*)-1);
  
  CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(pw_notifier_port), kCFRunLoopDefaultMode);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), lp_src, kCFRunLoopDefaultMode);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), pw_src, kCFRunLoopDefaultMode);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(dp_notifier_port), kCFRunLoopDefaultMode);
  IOObjectRelease(dp_root_port);
  CFRunLoopRun();
  
  return EXIT_SUCCESS;
}
