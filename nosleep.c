#include <stdio.h>
#include <termios.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

int main(int argc, const char* argv[]) {
	CFStringRef reasonForActivity = CFSTR("DON'T SLEEP!");
	IOPMAssertionID assertionID;

	if (IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn, reasonForActivity, &assertionID) != kIOReturnSuccess) {
		fprintf(stderr, "ERROR! Failed to prevent sleep");
	}

	printf("Sleep prevented! Press any key to stop...\n");

	struct termios info;
	tcgetattr(0, &info);
	info.c_lflag &= ~ICANON;
	info.c_cc[VMIN] = 1;
	info.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &info);

	getchar();

	IOPMAssertionRelease(assertionID);
	return 0;
}
