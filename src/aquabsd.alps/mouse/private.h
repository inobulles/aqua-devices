#define dynamic

#include <aquabsd.alps/mouse/public.h>

#include <stdio.h>
#include <stdint.h>

#define BUTTON_COUNT AQUABSD_ALPS_MOUSE_BUTTON_COUNT
#define AXIS_COUNT AQUABSD_ALPS_MOUSE_AXIS_COUNT

#define button_t aquabsd_alps_mouse_button_t
#define axis_t aquabsd_alps_mouse_axis_t

#define BUTTON_LEFT AQUABSD_ALPS_MOUSE_BUTTON_LEFT
#define BUTTON_RIGHT AQUABSD_ALPS_MOUSE_BUTTON_RIGHT
#define BUTTON_MIDDLE AQUABSD_ALPS_MOUSE_BUTTON_MIDDLE

#define AXIS_X AQUABSD_ALPS_MOUSE_AXIS_X
#define AXIS_Y AQUABSD_ALPS_MOUSE_AXIS_Y
#define AXIS_Z AQUABSD_ALPS_MOUSE_AXIS_Z

#define update_callback_t aquabsd_alps_mouse_update_callback_t
#define mouse_t aquabsd_alps_mouse_t

static unsigned default_mouse_id;

static unsigned mouse_count;
static mouse_t* mice;

#if defined(AQUABSD_ALPS_MOUSE_AQUABSD_CONSOLE_MOUSE)
	static unsigned aquabsd_console_mouse_id;
	#define AQUABSD_CONSOLE_MOUSE

	#include <aquabsd.alps/mouse/aquabsd_console_mouse.h>
#endif
