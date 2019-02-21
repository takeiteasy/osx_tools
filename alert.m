//
//  alert.m
//  alert
//
//  Created by Rory B. Bellows on 20/02/2019.
//  Copyright © 2019 Rory B. Bellows. All rights reserved.
//
// clang alert.m -framework AppKit


#include <AppKit/AppKit.h>
#include <Availability.h>

typedef enum {
  DIALOG_INFO,
  DIALOG_WARNING,
  DIALOG_ERROR,
  DIALOG_NOT_SET
} DIALOG_TYPE;

int main(int argc, char** argv) {
  @autoreleasepool {
    NSAlert* alert = [[NSAlert alloc] init];
    int opt;
    char* msg = NULL;
    extern char* optarg;
    extern int optopt;
    DIALOG_TYPE type = DIALOG_NOT_SET;
    
    while ((opt = getopt(argc, argv, ":iweocynm:x:")) != -1) {
      switch (opt) {
        case 'i':
          type = DIALOG_INFO;
          break;
        case 'w':
          type = DIALOG_WARNING;
          break;
        case 'e':
          type = DIALOG_ERROR;
        case 'o':
          [alert addButtonWithTitle:@"OK"];
          break;
        case 'c':
          [alert addButtonWithTitle:@"Cancel"];
          break;
        case 'y':
          [alert addButtonWithTitle:@"Yes"];
          break;
        case 'n':
          [alert addButtonWithTitle:@"No"];
          break;
        case 'm':
          msg = optarg;
          break;
        case 'x':
          [alert addButtonWithTitle:@(optarg)];
          break;
        case ':':
          printf("ERROR: \"-%c\" requires an argument!\n", optopt);
          return EXIT_FAILURE;
        case '?':
          printf("ERROR: Unknown arg \"-%c\"\n", optopt);
          break;
      }
    }
    
    switch (type) {
      case DIALOG_INFO:
        [alert setAlertStyle:NSAlertStyleInformational];
        break;
      case DIALOG_WARNING:
        [alert setAlertStyle:NSAlertStyleWarning];
        break;
      case DIALOG_ERROR:
        [alert setAlertStyle:NSAlertStyleCritical];
        break;
      case DIALOG_NOT_SET:
      default:
        printf("ERROR! Dialog type not set or invalid!\n");
        return EXIT_FAILURE;
    }
    
    if (!msg) {
      printf("ERROR! Message not set!\n");
      return EXIT_FAILURE;
    }
    [alert setMessageText:@(msg)];
    
    printf("%ld\n", [alert runModal] - 1000);
  }
  return EXIT_SUCCESS;
}
