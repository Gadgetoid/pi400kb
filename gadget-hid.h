#include <linux/hidraw.h>
#include <linux/usb/ch9.h>
#include <usbg/usbg.h>
#include <usbg/function/hid.h>

struct hid_buf {
    unsigned char data[64];
}  __attribute__ ((aligned (1)));


struct HIDDevice {
    int hidraw_fd;
    int hidraw_index;

    int uinput_fd;

    int output_fd;

    bool is_keyboard;

    int report_size;
    struct hidraw_report_descriptor report_desc;

    struct hid_buf buf;

    struct HIDDevice *next;
};

int initUSB(struct HIDDevice *devices);
int cleanupUSB();
