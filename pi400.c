#include "pi400.h"

#include "gadget-hid.h"

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <unistd.h>


#define HID_REPORT_SIZE 8

int hid_output;
int running = 0;
int key_index = 0;

void signal_handler(int dummy) {
    running = 0;
}

int main() {
    int ret;
    int fd;
    unsigned char buf[HID_REPORT_SIZE];
    chdir(KEYBOW_HOME);

    if ((fd = open(KEYBOARD_DEVICE, O_RDWR)) == -1) {
	printf("Failed to open keyboard device\n");
	return 1;
    }

    ret = initUSB();

    do {
        hid_output = open("/dev/hidg0", O_WRONLY | O_NDELAY);
    } while (hid_output == -1 && errno == EINTR);
    if (hid_output == -1){
        printf("Error opening /dev/hidg0 for writing.\n");
        return 1;
    }

    int x = 0;
    for(x = 0; x < NUM_KEYS; x++){
        last_state[x] = 0;
    }

    printf("Running...\n");
    running = 1;
    signal(SIGINT, signal_handler);

    while (running){
	read(fd, buf, HID_REPORT_SIZE);
	for(int x = 0; x < HID_REPORT_SIZE; x++)
	{
		printf("%x ", buf[x]);
	}
	printf("\n");
	write(hid_output, buf, HID_REPORT_SIZE);
        usleep(1000);
    } 

    printf("Cleanup USB\n");
    cleanupUSB();

    return 0;
}
