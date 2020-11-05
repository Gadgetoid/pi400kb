#include <bcm2835.h>
#include <pthread.h>

#define NUM_KEYS 12

#ifndef KEYBOW_HOME
#define KEYBOW_HOME "/boot/"
#endif

#define KEYBOARD_DEVICE "/dev/hidraw0"

pthread_mutex_t lights_mutex;

unsigned short last_state[NUM_KEYS];

int initUSB();
int main();
void sendHIDReport();
