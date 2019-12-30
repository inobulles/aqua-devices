
#include <stdint.h>
#include <string.h>

uint8_t* window_based_mouse_pointer;

int64_t* mouse_x_pointer;
int64_t* mouse_y_pointer;

int64_t* mouse_scroll_pointer;
uint8_t* mouse_button_pointer;

uint64_t** kos_bda_pointer = (uint64_t**) 0;

#define MOUSE_BUTTON_LEFT   0x00
#define MOUSE_BUTTON_MIDDLE 0x01
#define MOUSE_BUTTON_RIGHT  0x02

#define MOUSE_AXIS_X        0x00
#define MOUSE_AXIS_Y        0x01
#define MOUSE_AXIS_SCROLL   0x02

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	
	uint8_t window_based_mouse = *window_based_mouse_pointer;
	
	int64_t mouse_x = *mouse_x_pointer;
	int64_t mouse_y = *mouse_y_pointer;
	
	int64_t mouse_scroll = *mouse_scroll_pointer;
	uint8_t mouse_button = *mouse_button_pointer;
	
	if (data[0] == 0x63) { // count
		if (window_based_mouse) {
			kos_bda[0] = 1;
			
		} else {
			/// TODO
		}
		
	} else if (data[0] == 0x6E) { // name
		uint64_t mouse_id = data[1];
		
		if (window_based_mouse) {
			strcpy((char*) kos_bda, "window based mouse");
			
		} else {
			/// TODO
		}
		
	} else if (data[0] == 0x62) { // button
		uint64_t mouse_id = data[1];
		uint64_t button_id = data[2];
		
		if (window_based_mouse) {
			kos_bda[0] = (mouse_button >> button_id) & 1;
			
		} else {
			/// TODO
		}
		
	} else if (data[0] == 0x61) { // axis
		uint64_t mouse_id = data[1];
		uint64_t axis_id = data[2];
		
		if (window_based_mouse) {
			if      (axis_id == MOUSE_AXIS_X     ) kos_bda[0] = mouse_x;
			else if (axis_id == MOUSE_AXIS_Y     ) kos_bda[0] = mouse_y;
			else if (axis_id == MOUSE_AXIS_SCROLL) kos_bda[0] = mouse_scroll;
			
		} else {
			/// TODO
		}
	}
}

