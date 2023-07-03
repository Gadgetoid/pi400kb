#include <linux/hidraw.h>
#include <linux/usb/ch9.h>
#include <usbg/usbg.h>
#include <usbg/function/hid.h>

//#define KEYBOARD_VID             0x04d9
//#define KEYBOARD_PID             0x0007
//#define KEYBOARD_DEV             "/dev/input/by-id/usb-_Raspberry_Pi_Internal_Keyboard-event-kbd"
#define KEYBOARD_HID_REPORT_SIZE 8

//#define MOUSE_VID                0x093a
//#define MOUSE_PID                0x2510
//#define MOUSE_DEV                "/dev/input/by-id/usb-PixArt_USB_Optical_Mouse-event-mouse"
#define MOUSE_HID_REPORT_SIZE    4

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
