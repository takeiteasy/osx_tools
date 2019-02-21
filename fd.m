//
//  fd.m
//  fd
//
//  Created by Rory B. Bellows on 20/02/2019.
//  Copyright © 2019 Rory B. Bellows. All rights reserved.
//
// clang fd.m -framework AppKit


#include <AppKit/AppKit.h>
#include <Availability.h>

typedef enum {
  DIALOG_OPEN,
  DIALOG_OPEN_DIR,
  DIALOG_SAVE,
  DIALOG_NOT_SET
} DIALOG_ACTION;

int main(int argc, char** argv) {
  @autoreleasepool {
    NSSavePanel* panel;
    NSOpenPanel* open_panel;
    int opt;
    BOOL allowed_multiple = NO;
    char *pattern = NULL, *path = NULL, *filename = NULL;
    extern char* optarg;
    extern int optind, optopt, opterr;
    DIALOG_ACTION action = DIALOG_NOT_SET;
    
    while ((opt = getopt(argc, argv, ":odsmf:p:n:")) != -1) {
      switch(opt) {
        case 'o':
          action = DIALOG_OPEN;
          break;
        case 'd':
          action = DIALOG_OPEN_DIR;
          break;
        case 's':
          action = DIALOG_SAVE;
          break;
        case 'm':
          allowed_multiple = YES;
          break;
        case 'f':
          pattern = optarg;
          break;
        case 'p':
          path = optarg;
          break;
        case 'n':
          filename = optarg;
          break;
        case ':':
          printf("ERROR: \"-%c\" requires an argument!\n", optopt);
          return EXIT_FAILURE;
        case '?':
          printf("ERROR: Unknown arg \"-%c\"\n", optopt);
          break;
      }
    }
    
    switch (action) {
      case DIALOG_OPEN:
      case DIALOG_OPEN_DIR:
        open_panel = [NSOpenPanel openPanel];
        panel = open_panel;
        break;
      case DIALOG_SAVE:
        panel = [NSSavePanel savePanel];
        break;
      case DIALOG_NOT_SET:
      default:
        fprintf(stderr, "ERROR! No flag set\n");
        return EXIT_FAILURE;
    }
    [panel setLevel:CGShieldingWindowLevel()];
    
    if (!pattern || action == DIALOG_SAVE)
      goto SKIP_FILTERS;
    
    NSMutableArray* file_types = [[NSMutableArray alloc] init];
    char *token = strtok(pattern, ",");
    while (token) {
      [file_types addObject:[NSString stringWithUTF8String:token]];
      token = strtok(NULL, ",");
    }
    [panel setAllowedFileTypes:file_types];
    
  SKIP_FILTERS:
    if (path) {
      NSString *path_str = [NSString stringWithUTF8String:path];
      NSURL *path_url = [NSURL fileURLWithPath:path_str];
      panel.directoryURL = path_url;
    }
    
    if (filename) {
      NSString *filenameString = [NSString stringWithUTF8String:filename];
      panel.nameFieldStringValue = filenameString;
    }
    
    switch (action) {
      case DIALOG_OPEN:
        open_panel.allowsMultipleSelection = allowed_multiple;
        open_panel.canChooseDirectories = NO;
        open_panel.canChooseFiles = YES;
        break;
      case DIALOG_OPEN_DIR:
        open_panel.allowsMultipleSelection = allowed_multiple;
        open_panel.canCreateDirectories = YES;
        open_panel.canChooseDirectories = YES;
        open_panel.canChooseFiles = NO;
        break;
      case DIALOG_SAVE:
        break;
      case DIALOG_NOT_SET:
      default:
        return EXIT_FAILURE;
    }
    
    // Mute stderr to silence annoying warning by OSX
    int old_stderr = dup(2);
    freopen("/dev/null", "w", stderr);
    fclose(stderr);
                    
    if ([panel runModal] == NSModalResponseOK) {
      // Restore stderr
      stderr = fdopen(old_stderr, "w");
      
      if (action == DIALOG_SAVE || !allowed_multiple) {
        const char* url = [[[panel URL] path] UTF8String];
        if (!url)
          return EXIT_FAILURE;
        
        printf("%s\n", url);
      } else {
        NSArray* urls = [open_panel URLs];
        if (!urls)
          return EXIT_FAILURE;
        
        for (NSURL* url in urls)
          printf("%s\n", [[url path] UTF8String]);
      }
    } else
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
