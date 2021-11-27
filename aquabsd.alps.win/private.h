#define dynamic

#include <aquabsd.alps.win/public.h>

static uint64_t (*kos_query_device) (uint64_t, uint64_t name);
static void* (*kos_load_device_function) (uint64_t device, const char* name);
static uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

static uint64_t mouse_device = -1;
static uint64_t kbd_device = -1;

#define cb_t aquabsd_alps_win_cb_t

#define CB_DRAW AQUABSD_ALPS_WIN_CB_DRAW
#define CB_LEN AQUABSD_ALPS_WIN_CB_LEN

#define win_t aquabsd_alps_win_t
