#include <stdlib.h>
#include <string.h>

// functions accessible to AQUA programs and other devices

dynamic unsigned get_default_mouse_id(void) {
	return default_mouse_id;
}

dynamic int update_mouse(unsigned mouse_id) {
	mouse_t* mouse = &mice[mouse_id];

	if (!mouse->update_callback) {
		return 0;
	}

	return mouse->update_callback(mouse, mouse->update_cb_param);
}

dynamic unsigned poll_button(unsigned mouse_id, button_t button) {
	return mice[mouse_id].buttons[button];
}

dynamic float poll_axis(unsigned mouse_id, axis_t axis) {
	return mice[mouse_id].axes[axis];
}

// functions exclusively accessible to other devices

dynamic mouse_t* register_mouse(const char* name, update_callback_t update_callback, void* update_cb_param, unsigned set_default) {
	unsigned mouse_id = mouse_count++;

	mice = realloc(mice, mouse_count * sizeof *mice);
	mouse_t* mouse = &mice[mouse_id];
	
	memset(mouse, 0, sizeof *mouse);

	mouse->update_callback = update_callback;
	mouse->update_cb_param = update_cb_param;

	mouse->id = mouse_id;
	strncpy(mouse->name, name, sizeof mouse->name);

	if (set_default) {
		default_mouse_id = mouse_id;
	}
	
	return mouse;
}
