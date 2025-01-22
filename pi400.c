#include "pi400.h"

#include "gadget-hid.h"

#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define EVIOC_GRAB 1
#define EVIOC_UNGRAB 0

int hid_output;
volatile int running = 0;
volatile int grabbed = 0;

int ret;
int keyboard_fd;
int mouse_fd;
int uinput_keyboard_fd;
int uinput_mouse_fd;
struct hid_buf keyboard_buf;
struct hid_buf mouse_buf;

void signal_handler(int dummy) {
    running = 0;
}

bool modprobe_libcomposite() {
    pid_t pid;

    pid = fork();

    if (pid < 0) return false;
    if (pid == 0) {
        char* const argv[] = {"modprobe", "libcomposite", NULL};
        execv("/usr/sbin/modprobe", argv);
        exit(0);
    }
    waitpid(pid, NULL, 0);
}

bool trigger_hook() {
    char buf[4096];
    snprintf(buf, sizeof(buf), "%s %u", HOOK_PATH, grabbed ? 1u : 0u);
    system(buf);
}

int find_hidraw_device(char *device_type, int16_t vid, int16_t pid) {
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

        if(hidinfo.vendor == vid && hidinfo.product == pid) {
            printf("Found %s at: %s\n", device_type, path);
            return fd;
        }

        close(fd);
    }

    return -1;
}

int grab(char *dev) {
    printf("Grabbing: %s\n", dev);
    int fd = open(dev, O_RDONLY);
    ioctl(fd, EVIOCGRAB, EVIOC_UNGRAB);
    usleep(500000);
    ioctl(fd, EVIOCGRAB, EVIOC_GRAB);
    return fd;
}

void ungrab(int fd) {
    ioctl(fd, EVIOCGRAB, EVIOC_UNGRAB);
    close(fd);
}

void printhex(unsigned char *buf, size_t len) {
    for(int x = 0; x < len; x++)
    {
        printf("%x ", buf[x]);
    }
    printf("\n");
}

void ungrab_both() {
    printf("Releasing Keyboard and/or Mouse\n");

    if(uinput_keyboard_fd > -1) {
        ungrab(uinput_keyboard_fd);
    }

    if(uinput_mouse_fd > -1) {
        ungrab(uinput_mouse_fd);
    }

    grabbed = 0;

    trigger_hook();
}

void grab_both() {
    printf("Grabbing Keyboard and/or Mouse\n");

    if(keyboard_fd > -1) {
        uinput_keyboard_fd = grab(KEYBOARD_DEV);
    }

    if(mouse_fd > -1) {
        uinput_mouse_fd = grab(MOUSE_DEV);
    }

    if (uinput_keyboard_fd > -1 || uinput_mouse_fd > -1) {
        grabbed = 1;
    }

    trigger_hook();
}

void send_empty_hid_reports_both() {
    if(keyboard_fd > -1) {
#ifndef NO_OUTPUT
        memset(keyboard_buf.data, 0, KEYBOARD_HID_REPORT_SIZE);
        write(hid_output, (unsigned char *)&keyboard_buf, KEYBOARD_HID_REPORT_SIZE + 1);
#endif
    }

    if(mouse_fd > -1) {
#ifndef NO_OUTPUT
        memset(mouse_buf.data, 0, MOUSE_HID_REPORT_SIZE);
        write(hid_output, (unsigned char *)&mouse_buf, MOUSE_HID_REPORT_SIZE + 1);
#endif
    }
}

int main() {
    modprobe_libcomposite();

    keyboard_buf.report_id = 1;
    mouse_buf.report_id = 2;

    keyboard_fd = find_hidraw_device("keyboard", KEYBOARD_VID, KEYBOARD_PID);
    if(keyboard_fd == -1) {
        printf("Failed to open keyboard device\n");
    }
    
    mouse_fd = find_hidraw_device("mouse", MOUSE_VID, MOUSE_PID);
    if(mouse_fd == -1) {
        printf("Failed to open mouse device\n");
    }

    if(mouse_fd == -1 && keyboard_fd == -1) {
        printf("No devices to forward, bailing out!\n");
        return 1;
    }

#ifndef NO_OUTPUT
    ret = initUSB();
    if(ret != USBG_SUCCESS && ret != USBG_ERROR_EXIST) {
        return 1;
    }
#endif

    grab_both();


#ifndef NO_OUTPUT
    do {
        hid_output = open("/dev/hidg0", O_WRONLY | O_NDELAY);
    } while (hid_output == -1 && errno == EINTR);

    if (hid_output == -1){
        printf("Error opening /dev/hidg0 for writing.\n");
        return 1;
    }
#endif

    printf("Running...\n");
    running = 1;
    signal(SIGINT, signal_handler);

    struct pollfd pollFd[2];
    pollFd[0].fd = keyboard_fd;
    pollFd[0].events = POLLIN;
    pollFd[1].fd = mouse_fd;
    pollFd[1].events = POLLIN;

    while (running){
        poll(pollFd, 2, -1);
        if(keyboard_fd > -1) {
            int c = read(keyboard_fd, keyboard_buf.data, KEYBOARD_HID_REPORT_SIZE);

            if(c == KEYBOARD_HID_REPORT_SIZE){
                printf("K:");
                printhex(keyboard_buf.data, KEYBOARD_HID_REPORT_SIZE);

#ifndef NO_OUTPUT
                if(grabbed) {
                    write(hid_output, (unsigned char *)&keyboard_buf, KEYBOARD_HID_REPORT_SIZE + 1);
                    usleep(1000);
                }
#endif

                // Trap Ctrl + Raspberry and toggle capture on/off
                if(keyboard_buf.data[0] == 0x09){
                    if(grabbed) {
                        ungrab_both();
                        send_empty_hid_reports_both();
                    } else {
                        grab_both();
                    }
                }
                // Trap Ctrl + Shift + Raspberry and exit
                if(keyboard_buf.data[0] == 0x0b){
                    running = 0;
                    break;
                }
            }
        }
        if(mouse_fd > -1) {
            int c = read(mouse_fd, mouse_buf.data, MOUSE_HID_REPORT_SIZE);

            if(c == MOUSE_HID_REPORT_SIZE){
                printf("M:");
                printhex(mouse_buf.data, MOUSE_HID_REPORT_SIZE);

#ifndef NO_OUTPUT
                if(grabbed) {
                    write(hid_output, (unsigned char *)&mouse_buf, MOUSE_HID_REPORT_SIZE + 1);
                    usleep(1000);
                }
#endif
            }
        }
    }

    ungrab_both();
    send_empty_hid_reports_both();

#ifndef NO_OUTPUT
    printf("Cleanup USB\n");
    cleanupUSB();
#endif

    return 0;
}
