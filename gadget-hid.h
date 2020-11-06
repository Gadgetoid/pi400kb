#include <linux/usb/ch9.h>
#include <usbg/usbg.h>
#include <usbg/function/hid.h>

#define VENDOR          0x04d9
#define PRODUCT         0x0007
#define HID_REPORT_SIZE 8

usbg_state *s;
usbg_gadget *g;
usbg_config *c;
usbg_function *f_hid;

int initUSB();
int cleanupUSB();
