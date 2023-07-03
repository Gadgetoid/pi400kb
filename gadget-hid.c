#include "gadget-hid.h"
#include <errno.h>
#include <stdio.h>
#include <linux/usb/ch9.h>
#include <usbg/usbg.h>
#include <usbg/function/hid.h>

usbg_state *s;
usbg_gadget *g;
usbg_config *c;
usbg_function *f_hid;

static char report_desc[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x03,        //   Report Count (3)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x03,        //   Usage Maximum (Scroll Lock)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection

    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x03,        //     Usage Maximum (0x03)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x05,        //     Report Size (5)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection
};

static void print_usbg_error(const char *str, int usbg_ret) {
    fprintf(stderr, "Error %s: %s : %s\n", str, usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
}

int initUSB() {
    int usbg_ret = -EINVAL;

    struct usbg_gadget_attrs g_attrs = {
        .bcdUSB = 0x0200,
        .bDeviceClass = USB_CLASS_PER_INTERFACE,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
        .idVendor = KEYBOARD_VID,
        .idProduct = KEYBOARD_PID,
        .bcdDevice = 0x0001, /* Verson of device */
    };

    struct usbg_gadget_strs g_strs = {
        .serial = "0123456789",      /* Serial number */
        .manufacturer = "Gadgetoid", /* Manufacturer */
        .product = "Pi400KB"         /* Product string */
    };

    struct usbg_config_strs c_strs = {
        .configuration = "1xHID"
    };

    struct usbg_f_hid_attrs f_attrs = {
        .protocol = 1,
        .report_desc = {
            .desc = report_desc,
            .len = sizeof(report_desc),
        },
        .report_length = 16,
        .subclass = 0,
    };

    usbg_ret = usbg_init("/sys/kernel/config", &s);
    if (usbg_ret != USBG_SUCCESS)
        print_usbg_error("on usbg init", usbg_ret);

    // check for existing gadget
    usbg_gadget *ex_gadget = usbg_get_gadget(s, "g1");

    if(ex_gadget) {
        // remove it (assume failed shutdown)
        usbg_disable_gadget(ex_gadget);
        usbg_rm_gadget(ex_gadget, USBG_RM_RECURSE);
    }

    usbg_ret = usbg_create_gadget(s, "g1", &g_attrs, &g_strs, &g);
    if (usbg_ret != USBG_SUCCESS) {
        print_usbg_error("creating gadget", usbg_ret);
        
        usbg_cleanup(s);
        s = NULL;
    }

    usbg_ret = usbg_create_function(g, USBG_F_HID, "usb0", &f_attrs, &f_hid);
    if (usbg_ret != USBG_SUCCESS) {
        print_usbg_error("creating function: USBG_F_HID", usbg_ret);
        cleanupUSB();
    }

    usbg_ret = usbg_create_config(g, 1, "config", NULL, &c_strs, &c);
    if (usbg_ret != USBG_SUCCESS) {
        print_usbg_error("creating config", usbg_ret);
        cleanupUSB();
    }

    usbg_ret = usbg_add_config_function(c, "keyboard", f_hid);
    if (usbg_ret != USBG_SUCCESS) {
        print_usbg_error("adding function: keyboard", usbg_ret);
        cleanupUSB();
    }

    usbg_ret = usbg_enable_gadget(g, DEFAULT_UDC);
    if (usbg_ret != USBG_SUCCESS) {
        print_usbg_error("enabling gadget", usbg_ret);
        cleanupUSB();
    }

    return usbg_ret;
}

int cleanupUSB(){
    if(g){
        usbg_disable_gadget(g);
        usbg_rm_gadget(g, USBG_RM_RECURSE);
        g = NULL;
    }
    if(s){
        usbg_cleanup(s);
        s = NULL;
    }
    return 0;
}
