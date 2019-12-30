
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <linux/joystick.h>

#define PRECISION 1000000

#define MAX_JOYSTICK_COUNT 16
#define MAX_JOYSTICK_AXIS_COUNT 16

static uint64_t joystick_count = 0;
static struct { int fd; uint64_t button_state; int16_t axes[MAX_JOYSTICK_AXIS_COUNT]; } joysticks[MAX_JOYSTICK_COUNT] = { 0 };

static uint64_t __attribute__((unused)) joystick_axis_count(int fd) {
	uint8_t count;
	return ioctl(fd, JSIOCGAXES, &count) < 0 ? -1 : count;
}

static uint64_t __attribute__((unused)) joystick_button_count(int fd) {
	uint8_t count;
	return ioctl(fd, JSIOCGBUTTONS, &count) < 0 ? -1 : count;
}

void after_flip(void) {
	joystick_count = 0;
	
	for (int i = 0; i < MAX_JOYSTICK_COUNT; i++) {
		char path[128];
		sprintf(path, "/dev/input/js%d", i);
		
		int fd = joysticks[i].fd;
		if (fd < 0) {
			if ((fd = joysticks[i].fd = open(path, O_RDONLY | O_NONBLOCK)) < 0) {
				break;
			}
		}
		
		joystick_count++;
		
		while (1) {
			struct js_event event_buffer[0xFF];
			int bytes = read(fd, event_buffer, sizeof event_buffer);
			
			if (bytes < 0) {
				if (errno != EAGAIN) { // joystick unplugged
					printf("Joystick %d unplugged\n", i);
					memset(&joysticks[i], 0, sizeof joysticks[i]);
					joysticks[i].fd = -1;
				}
				
				break;
			}
			
			uint64_t button_state = joysticks[i].button_state; // no idea why I have to do this
			int16_t axes[MAX_JOYSTICK_AXIS_COUNT];
			memcpy(axes, joysticks[i].axes, sizeof axes);
			
			for (int i = 0; i < bytes / sizeof *event_buffer; i++) {
				struct js_event* event = &event_buffer[i];
				event->type &= ~JS_EVENT_INIT;
				
				if (event->type == JS_EVENT_BUTTON) {
					if (event->value) button_state |=  (1 << event->number);
					else              button_state &= ~(1 << event->number);
					
				} else if (event->type == JS_EVENT_AXIS) {
					axes[event->number] = event->value;
				}
			}
			
			joysticks[i].button_state = button_state;
			memcpy(joysticks[i].axes, axes, sizeof axes);
		}
	}
}

void load(void) {
	for (int i = 0; i < MAX_JOYSTICK_COUNT; i++) {
		joysticks[i].fd = -1;
	}
	
	after_flip();
}

void quit(void) {
	for (int i = 0; i < MAX_JOYSTICK_COUNT; i++) if (joysticks[i].fd >= 0) {
		close(joysticks[i].fd);
	}
}

uint64_t* kos_bda_bytes_pointer = (uint64_t*) 0;
uint64_t** kos_bda_pointer = (uint64_t**) 0;

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t kos_bda_bytes = *kos_bda_bytes_pointer;
	uint64_t* kos_bda = *kos_bda_pointer;
	
	*result_pointer_pointer = &kos_bda[0];
	
	if (data[0] == 0x63) { // count
		kos_bda[0] = joystick_count;
		
	} else if (data[0] == 0x6E) { // name
		uint64_t joystick_id = data[1];
		
		if (!joysticks[joystick_id].fd) strncpy((char*) kos_bda, "inexistent", kos_bda_bytes);
		if (ioctl(joysticks[joystick_id].fd, JSIOCGNAME(kos_bda_bytes), (char*) kos_bda) < 0) strncpy((char*) kos_bda, "unknown", kos_bda_bytes);
		
	} else if (data[0] == 0x62) { // button
		uint64_t joystick_id = data[1];
		uint64_t button_id = data[2];
		
		kos_bda[0] = !!(joysticks[joystick_id].button_state & (1 << button_id));
		
	} else if (data[0] == 0x61) { // axis
		uint64_t joystick_id = data[1];
		uint64_t axis_id = data[2];
		
		kos_bda[0] = (int64_t) (((float) joysticks[joystick_id].axes[axis_id] / 32768.0f) * PRECISION);
		
	} else if (data[0] == 0x68) { // rumble
		uint64_t joystick_id = data[1];
		uint64_t strength = data[2];
		uint64_t seconds = data[3];
		uint64_t period = data[4];
		uint64_t direction = data[5];
		
		/// TODO
	}
}

