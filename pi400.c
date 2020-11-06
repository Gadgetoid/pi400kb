#include "pi400.h"

#include "gadget-hid.h"

#include <sys/ioctl.h>
#include <linux/hidraw.h>
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
volatile int running = 0;
int key_index = 0;

void signal_handler(int dummy) {
    running = 0;
}

int find_hidraw_device() {
    int fd;
    int ret;
    struct hidraw_devinfo hidinfo;
    char path[20];

    for(int x = 0; x < 16; x++){
	sprintf(path, "/dev/hidraw%d", x);
        if ((fd = open(path, O_RDWR | O_NONBLOCK)) == -1) {
            continue;
        }

        ret = ioctl(fd, HIDIOCGRAWINFO, &hidinfo);

        if(hidinfo.vendor == VENDOR && hidinfo.product == PRODUCT) {
	    printf("Found keyboard at: %s\n", path);
	    return fd;
        }

	close(fd);
    }

    return -1;
}

int main() {
    int ret;
    int fd;
    unsigned char buf[HID_REPORT_SIZE];

    fd = find_hidraw_device();
    if(fd == -1) {
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

    printf("Running...\n");
    running = 1;
    signal(SIGINT, signal_handler);

    while (running){
	int c = read(fd, buf, HID_REPORT_SIZE);
	if(c != HID_REPORT_SIZE){
		continue;
	}
	for(int x = 0; x < HID_REPORT_SIZE; x++)
	{
		printf("%x ", buf[x]);
	}
	printf("\n");
	write(hid_output, buf, HID_REPORT_SIZE);
        usleep(1000);

	if(buf[0] == 0x09){
	    running = 0;
	    break;
	}
    } 

    for(int x = 0; x < HID_REPORT_SIZE; x++){
	buf[x] = 0;
    };

    write(hid_output, buf, HID_REPORT_SIZE);

    printf("Cleanup USB\n");
    cleanupUSB();

    return 0;
}
