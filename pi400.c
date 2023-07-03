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

volatile int running = 0;
volatile int grabbed = 0;

int ret;

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

static struct HIDDevice *devices = NULL;
static int num_devices = 0;

void ungrab(struct HIDDevice *device);

void signal_handler(int dummy) {
    running = 0;
}

bool modprobe_libcomposite() {
    pid_t pid;

    pid = fork();

    if (pid < 0) return false;
    if (pid == 0) {
        char* const argv[] = {"modprobe", "libcomposite"};
        execv("/usr/sbin/modprobe", argv);
        exit(0);
    }
    waitpid(pid, NULL, 0);
}

void trigger_hook() {
    if(access(HOOK_PATH, F_OK) != 0)
        return;

    char buf[4096];
    snprintf(buf, sizeof(buf), "%s %u", HOOK_PATH, grabbed ? 1u : 0u);
    system(buf);
}

static void init_hid_device(struct HIDDevice *dev) {
    dev->hidraw_fd = -1;
    dev->hidraw_index = -1;
    dev->uinput_fd = -1;
    dev->output_fd = -1;

    dev->is_keyboard = false;
    dev->report_size = 0;

    dev->next = NULL;
}

static void cleanup_hid_device(struct HIDDevice *dev) {
    if(dev->hidraw_fd != -1)
        close(dev->hidraw_fd);
    if(dev->uinput_fd != -1)
        ungrab(dev);
    if(dev->output_fd != -1)
        close(dev->output_fd);

    dev->hidraw_fd = -1;
    dev->uinput_fd = -1;
    dev->output_fd = -1;
}

bool find_hidraw_device(struct HIDDevice *device, char *device_type, int16_t vid, int16_t pid) {
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

        if(ret == 0 && hidinfo.vendor == vid && hidinfo.product == pid) {
            char name[256] = {0};
            ioctl(fd, HIDIOCGRAWNAME(sizeof(name)), &name);

            printf("Found %s at: %s (%s)\n", device_type, path, name);
            device->hidraw_fd = fd;
            device->hidraw_index = x;

            // grab the report descriptor
            int desc_size;
            ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
            device->report_desc.size = desc_size;

            ret = ioctl(fd, HIDIOCGRDESC, &device->report_desc);
            if(ret < 0) {
                printf("Failed to get report descriptor!\n");
                return false;
            }

            return true;
        }

        close(fd);
    }

    return false;
}

static bool find_event_path(struct HIDDevice *device, char *output, int out_len) {
    bool found = false;

    char path[266];
    snprintf(path, sizeof(path), "/sys/class/hidraw/hidraw%i/device/input/", device->hidraw_index);

    DIR *inputDir = opendir(path);

    if(!inputDir)
        return false;

    struct dirent *inputEnt, *eventEnt;

    // scan through inputN (assuming one)
    while((inputEnt = readdir(inputDir)) != NULL && !found) {
        if(strncmp(inputEnt->d_name, "input", 5) != 0)
            continue;

        snprintf(path, sizeof(path), "/sys/class/hidraw/hidraw%i/device/input/%s/", device->hidraw_index, inputEnt->d_name);

        DIR *eventDir = opendir(path);

        if(!eventDir)
            continue;

        // now scan for eventN (also assming one)
        while((eventEnt = readdir(eventDir)) != NULL && !found) {
            if(strncmp(eventEnt->d_name, "event", 5) != 0)
                continue;

            snprintf(path, sizeof(path), "/sys/class/hidraw/hidraw%i/device/input/%s/%s/uevent", device->hidraw_index, inputEnt->d_name, eventEnt->d_name);

            // parse the uevent file
            FILE *f = fopen(path, "r");

            if(!f)
                continue;

            int line_len;
            char *line = NULL;

            while((line_len = getline(&line, &line_len, f)) != -1) {
                // get DEVNAME
                if(strncmp(line, "DEVNAME=", 8) == 0) {
                    line[line_len - 1] = 0;
                    snprintf(output, out_len, "/dev/%s", line + 8);
                    found = true;
                    break;
                }
            }

            fclose(f);
        }

    }

    closedir(inputDir);

    return found;
}

bool grab(struct HIDDevice *device) {
    char path[256];

    if(device->hidraw_fd == -1)
        return false;

    if(!find_event_path(device, path, sizeof(path)))
        return false;

    printf("Grabbing: %s\n", path);
    int fd = open(path, O_RDONLY);
    ioctl(fd, EVIOCGRAB, EVIOC_UNGRAB);
    usleep(500000);
    ioctl(fd, EVIOCGRAB, EVIOC_GRAB);

    device->uinput_fd = fd;

    return true;
}

void ungrab(struct HIDDevice *device) {
    if(device->uinput_fd == -1)
        return;

    ioctl(device->uinput_fd, EVIOCGRAB, EVIOC_UNGRAB);
    close(device->uinput_fd);

    device->uinput_fd = -1;
}

void printhex(unsigned char *buf, size_t len) {
    for(int x = 0; x < len; x++)
    {
        printf("%x ", buf[x]);
    }
    printf("\n");
}

void ungrab_all() {
    printf("Releasing Input Devices\n");

    for(struct HIDDevice *d = devices; d; d = d->next)
        ungrab(d);

    grabbed = 0;

    trigger_hook();
}

void grab_all() {
    printf("Grabbing Input Devices\n");

    for(struct HIDDevice *d = devices; d; d = d->next) {
        if(grab(d))
            grabbed = true;
    }

    trigger_hook();
}

static bool open_output(struct HIDDevice *device, const char *path) {
    do {
        device->output_fd = open(path, O_WRONLY | O_NDELAY);
    } while (device->output_fd == -1 && errno == EINTR);

    if (device->output_fd == -1){
        printf("Error opening %s for writing.\n", path);
        return false;
    }
    return true;
}

void send_empty_hid_reports_all() {
#ifndef NO_OUTPUT
    for(struct HIDDevice *d = devices; d; d = d->next) {
        if(d->output_fd) {
            memset(d->buf.data, 0, d->report_size);
            write(d->output_fd, d->buf.data, d->report_size);
        }
    }
#endif
}

int main() {
    modprobe_libcomposite();

    struct HIDDevice *keyboard_device = malloc(sizeof(struct HIDDevice));
    struct HIDDevice *mouse_device = malloc(sizeof(struct HIDDevice));

    init_hid_device(keyboard_device);
    init_hid_device(mouse_device);

    devices = keyboard_device;
    keyboard_device->next = mouse_device;
    num_devices = 2;

    keyboard_device->is_keyboard = true;

    int found_devices = 0;

    if(find_hidraw_device(keyboard_device, "keyboard", KEYBOARD_VID, KEYBOARD_PID))
        found_devices++;
    else
        printf("Failed to open keyboard device\n");

    if(find_hidraw_device(mouse_device, "mouse", MOUSE_VID, MOUSE_PID))
        found_devices++;
    else
        printf("Failed to open mouse device\n");

    if(!found_devices) {
        printf("No devices to forward, bailing out!\n");
        return 1;
    }

#ifndef NO_OUTPUT
    ret = initUSB(&keyboard_device->report_desc, &mouse_device->report_desc);
    if(ret != USBG_SUCCESS && ret != USBG_ERROR_EXIST) {
        return 1;
    }
#endif

    grab_all();


#ifndef NO_OUTPUT
    // making some assumptions here...
    if(!open_output(keyboard_device, "/dev/hidg0") || !open_output(mouse_device, "/dev/hidg1"))
        return 1;
#endif

    printf("Running...\n");
    running = 1;
    signal(SIGINT, signal_handler);

    struct pollfd *poll_fds = malloc(sizeof(struct pollfd) * num_devices);

    int i = 0;
    for(struct HIDDevice *d = devices; d; d = d->next, i++) {
        poll_fds[i].fd = d->hidraw_fd;
        poll_fds[i].events = POLLIN;
    }

    while (running){
        poll(poll_fds, num_devices, -1);
        // should check which fds are ready...

        for(struct HIDDevice *d = devices; d; d = d->next, i++) {
            if(d->hidraw_fd == -1)
                continue;

            int c = read(d->hidraw_fd, d->buf.data, 64);

            if(c == -1)
                continue;

            d->report_size = c;

            printf("R:");
            printhex(d->buf.data, c);

#ifndef NO_OUTPUT
            if(grabbed) {
                write(d->output_fd, d->buf.data, c);
                usleep(1000);
            }
#endif

            if(d->is_keyboard) {
                // Trap Ctrl + Raspberry and toggle capture on/off
                if(d->buf.data[0] == 0x09){
                    if(grabbed) {
                        ungrab_all();
                        send_empty_hid_reports_all();
                    } else {
                        grab_all();
                    }
                }

                // Trap Ctrl + Shift + Raspberry and exit
                if(d->buf.data[0] == 0x0b){
                    running = 0;
                    break;
                }
            }
        }

    }

    free(poll_fds);

    ungrab_all();
    send_empty_hid_reports_all();

    cleanup_hid_device(keyboard_device);
    cleanup_hid_device(mouse_device);

    free(keyboard_device);
    free(mouse_device);

#ifndef NO_OUTPUT
    printf("Cleanup USB\n");
    cleanupUSB();
#endif

    return 0;
}
