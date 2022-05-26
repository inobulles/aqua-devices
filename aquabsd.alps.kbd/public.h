#if !defined(__AQUABSD_ALPS_KBD)
#define __AQUABSD_ALPS_KBD

// definitions accessible to AQUA programs (through 'aqua-lib') and other devices

// the 'aquabsd_alps_kbd_button_t' enum must be identical to the 'kbd_button_t' enum in 'aqua-lib' ('c/aquabsd/alps/kbd.h')
// 'aquabsd_alps_kbd_t' struct must be identical to the 'kbd_t' struct in 'aqua-lib' ('c/aquabsd/alps/kbd.h')

typedef enum {
	AQUABSD_ALPS_KBD_BUTTON_ESC,
	AQUABSD_ALPS_KBD_BUTTON_TAB,

	AQUABSD_ALPS_KBD_BUTTON_UP,
	AQUABSD_ALPS_KBD_BUTTON_DOWN,
	AQUABSD_ALPS_KBD_BUTTON_LEFT,
	AQUABSD_ALPS_KBD_BUTTON_RIGHT,

	AQUABSD_ALPS_KBD_BUTTON_COUNT,
} aquabsd_alps_kbd_button_t;

// definitions exclusively accessible to other devices

typedef struct aquabsd_alps_kbd_t aquabsd_alps_kbd_t; // forward declaration
typedef int (*aquabsd_alps_kbd_update_callback_t) (aquabsd_alps_kbd_t* kbd, void* param);

struct aquabsd_alps_kbd_t {
	unsigned available;

	aquabsd_alps_kbd_update_callback_t update_callback;
	void* update_cb_param;

	unsigned id;
	char name[256];

	unsigned buttons[AQUABSD_ALPS_KBD_BUTTON_COUNT];

	unsigned buf_len;
	void* buf;
};

#if defined(__FreeBSD__)
	#define AQUABSD_ALPS_KBD_AQUABSD_CONSOLE_KBD
#endif

static unsigned (*aquabsd_alps_kbd_get_default_kbd_id) (void);
static int (*aquabsd_alps_kbd_update_kbd) (unsigned kbd_id);

static unsigned (*aquabsd_alps_kbd_poll_button) (unsigned kbd_id, aquabsd_alps_kbd_button_t button);

static unsigned (*aquabsd_alps_kbd_get_buf_len) (unsigned kbd_id);
static int (*aquabsd_alps_kbd_read_buf) (unsigned kbd_id, void* buf);

static aquabsd_alps_kbd_t* (*aquabsd_alps_kbd_register_kbd) (const char* name, aquabsd_alps_kbd_update_callback_t update_callback, void* update_cb_param, unsigned set_default);

#endif
