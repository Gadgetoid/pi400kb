#include "gadget-hid.h"
#include <errno.h>
#include <stdio.h>
#include <linux/usb/ch9.h>
#include <usbg/usbg.h>
#include <usbg/function/hid.h>

usbg_state *s;
usbg_gadget *g;
usbg_config *c;

static void print_usbg_error(const char *str, int usbg_ret) {
    fprintf(stderr, "Error %s: %s : %s\n", str, usbg_error_name(usbg_ret),
            usbg_strerror(usbg_ret));
}

int initUSB(struct HIDDevice *devices) {
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

    usbg_ret = usbg_create_config(g, 1, "config", NULL, &c_strs, &c);
    if (usbg_ret != USBG_SUCCESS) {
        print_usbg_error("creating config", usbg_ret);
        cleanupUSB();
    }

    int i = 0;
    for(struct HIDDevice *d = devices; d; d = d->next, i++) {
        struct usbg_f_hid_attrs f_attrs = {
            .protocol = 1,
            .report_desc = {
                .desc = d->report_desc.value,
                .len = d->report_desc.size,
            },
            .report_length = 16,
            .subclass = 0,
        };

        usbg_function *func;

        char str[10];
        snprintf(str, sizeof(str), "hid%i", i);

        // create func
        usbg_ret = usbg_create_function(g, USBG_F_HID, str, &f_attrs, &func);
        if (usbg_ret != USBG_SUCCESS) {
            print_usbg_error("creating function: USBG_F_HID", usbg_ret);
            cleanupUSB();
        }

        // add func
        usbg_ret = usbg_add_config_function(c, str, func);
        if (usbg_ret != USBG_SUCCESS) {
            print_usbg_error("adding function", usbg_ret);
            cleanupUSB();
        }
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
