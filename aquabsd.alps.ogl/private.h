#define dynamic

#include <aquabsd.alps.ogl/public.h>

// TODO are 'kos_query_device' & 'kos_load_device_function' going to be necessary?

static uint64_t (*kos_query_device) (uint64_t, uint64_t name);
static void* (*kos_load_device_function) (uint64_t device, const char* name);
static uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

#define cb_t aquabsd_alps_ogl_cb_t

#define CB_DRAW AQUABSD_ALPS_OGL_CB_DRAW
#define CB_LEN AQUABSD_ALPS_OGL_CB_LEN

#define window_t aquabsd_alps_ogl_window_t
#define context_t aquabsd_alps_ogl_context_t
