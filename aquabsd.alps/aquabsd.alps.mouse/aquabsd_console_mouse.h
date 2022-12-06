// pertinent information concerning console mouse on aquaBSD/FreeBSD:
// https://forums.freebsd.org/threads/reading-dev-sysmouse-as-a-user.81970

#include <sys/fbio.h>
#include <sys/consio.h>

static inline int position_aquabsd_console_mouse(int x, int y) {
	mouse_info_t mouse_info = { .operation = MOUSE_MOVEABS };

	mouse_info.u.data.x = x;
	mouse_info.u.data.y = y;

	if (ioctl(0, CONS_MOUSECTL, &mouse_info) < 0) {
		return -1;
	}

	return 0;
}

static inline int __attribute__((unused)) /* currently have no use for this function ... */ init_aquabsd_console_mouse(void) {
	// centre mouse

	int video_mode;
	
	if (ioctl(0, CONS_GET, &video_mode) < 0) {
		return -1;
	}

	video_info_t mode_info = { .vi_mode = video_mode };

	if (ioctl(0, CONS_MODEINFO, &mode_info) < 0) {
		return -1;
	}

	if (position_aquabsd_console_mouse(mode_info.vi_width / 2, mode_info.vi_height / 2) < 0) {
		return -1;
	}

	return 0;
}

static inline int update_aquabsd_console_mouse(mouse_t* mouse, __attribute__((unused)) void* _) {
	// get resolution information
	
	int video_mode;

	if (ioctl(0, CONS_GET, &video_mode) < 0) {
		goto error;
	}

	video_info_t mode_info = { .vi_mode = video_mode };

	if (ioctl(0, CONS_MODEINFO, &mode_info) < 0) {
		goto error;
	}

	// get mouse info

	mouse_info_t mouse_info = { .operation = MOUSE_GETINFO };
	
	if (ioctl(0, CONS_MOUSECTL, &mouse_info) < 0) {
		goto error;
	}

	struct {
		unsigned left        : 1;
		unsigned middle      : 1;
		unsigned right       : 1;

		unsigned scroll_down : 1;
		unsigned scroll_up   : 1;
	} button;

	*(int*) &button = mouse_info.u.data.buttons;

	mouse->buttons[BUTTON_LEFT  ] = button.left;
	mouse->buttons[BUTTON_RIGHT ] = button.right;
	mouse->buttons[BUTTON_MIDDLE] = button.middle;

	mouse->axes[AXIS_X] =       (float) mouse_info.u.data.x / mode_info.vi_width;
	mouse->axes[AXIS_Y] = 1.0 - (float) mouse_info.u.data.y / mode_info.vi_height;

	mouse->axes[AXIS_Z] = 0;

	if (button.scroll_down) mouse->axes[AXIS_Z] -= 1.0;
	if (button.scroll_up  ) mouse->axes[AXIS_Z] += 1.0;

	// "reset" mouse
	// to be perfectly honest, I don't know why this works, but after a lot of trial and error, I've found it's the only way to get the two virtual "scrolling buttons" to actually register
	// the relevant source files are (read them at risk of your own sanity) 'sys/dev/syscons/scmouse.c' & 'usr.sbin/moused/moused.c'
	// I wish the people who wrote these monstrosities (especially 'scmouse.c') be sentenced to an eternel life of stubbing their toes against furniture
	
	mouse_info.operation = MOUSE_ACTION;
	mouse_info.u.data.buttons |= 0b11000; // set the two virtual "scrolling buttons"

	mouse_info.u.data.x = 0; // make sure we don't move around the mouse
	mouse_info.u.data.y = 0;

	if (ioctl(0, CONS_MOUSECTL, &mouse_info) < 0) {
		goto error;
	}

	mouse_info.operation = MOUSE_GETINFO;
	ioctl(0, CONS_MOUSECTL, &mouse_info); // unnecessary to check for errors here

	return 0;

error:

	mouse->available = 0;
	return -1;
}
