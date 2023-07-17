#define dynamic

#include <aquabsd.alps.kbd/public.h>
#include <aquabsd.alps.kbd/map/map.h>

#include <stdio.h>
#include <stdint.h>

#include <umber.h>
#define UMBER_COMPONENT "aquabsd.alps.kbd"

#define BUTTON_COUNT AQUABSD_ALPS_KBD_BUTTON_COUNT

#define update_callback_t aquabsd_alps_kbd_update_callback_t
#define kbd_t aquabsd_alps_kbd_t

static unsigned default_kbd_id;

static unsigned kbd_count;
static kbd_t* kbds;

#if defined(AQUABSD_ALPS_KBD_AQUABSD_CONSOLE_KBD)
	static unsigned aquabsd_console_kbd_id;
	#define AQUABSD_CONSOLE_KBD

	#include <aquabsd.alps.kbd/aquabsd_console_kbd.h>
#endif
