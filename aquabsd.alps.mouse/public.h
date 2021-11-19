#if !defined(__AQUABSD_ALPS_MOUSE)
#define __AQUABSD_ALPS_MOUSE

// definitions accessible to AQUA programs (through 'aqua-lib') and other devices

// 'aquabsd_alps_mouse_button_t' & 'aquabsd_alps_mouse_axis_t' enums must respectively be identical to the 'mouse_button_t' & 'mouse_axis_t' enums in 'aqua-lib' ('c/aquabsd/alps/mouse.h')
// 'aquabsd_alps_mouse_t' struct must be identical to the 'mouse_t' struct in 'aqua-lib' ('c/aquabsd/alps/mouse.h')

typedef enum {
	AQUABSD_ALPS_MOUSE_BUTTON_LEFT,
	AQUABSD_ALPS_MOUSE_BUTTON_RIGHT,
	AQUABSD_ALPS_MOUSE_BUTTON_MIDDLE,

	AQUABSD_ALPS_MOUSE_BUTTON_COUNT,
} aquabsd_alps_mouse_button_t;

typedef enum {
	AQUABSD_ALPS_MOUSE_AXIS_X,
	AQUABSD_ALPS_MOUSE_AXIS_Y,
	AQUABSD_ALPS_MOUSE_AXIS_Z, // also known as the 'SCROLL' axis

	AQUABSD_ALPS_MOUSE_AXIS_COUNT,
} aquabsd_alps_mouse_axis_t;

// definitions exclusively accessible to other devices

typedef struct aquabsd_alps_mouse_t aquabsd_alps_mouse_t; // forward declaration
typedef int (*aquabsd_alps_mouse_update_callback_t) (aquabsd_alps_mouse_t* mouse, void* param);

struct aquabsd_alps_mouse_t {
	unsigned available;

	aquabsd_alps_mouse_update_callback_t update_callback;
	void* update_cb_param;

	unsigned id;
	char name[256];

	unsigned buttons[AQUABSD_ALPS_MOUSE_BUTTON_COUNT];
	float axes[AQUABSD_ALPS_MOUSE_AXIS_COUNT];
};

#if defined(__FreeBSD__)
	#define AQUABSD_ALPS_MOUSE_AQUABSD_CONSOLE_MOUSE
#endif

static unsigned (*aquabsd_alps_mouse_get_default_mouse_id) (void);
static int (*aquabsd_alps_mouse_update_mouse) (unsigned mouse_id);

static unsigned (*aquabsd_alps_mouse_poll_button) (unsigned mouse_id, aquabsd_alps_mouse_button_t button);
static float (*aquabsd_alps_mouse_poll_axis) (unsigned mouse_id, aquabsd_alps_mouse_axis_t axis);

static aquabsd_alps_mouse_t* (*aquabsd_alps_mouse_register_mouse) (const char* name, aquabsd_alps_mouse_update_callback_t update_callback, void* update_cb_param, unsigned set_default);

#endif
