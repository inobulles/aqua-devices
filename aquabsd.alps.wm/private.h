#define dynamic

#include <aquabsd.alps.wm/public.h>

static uint64_t (*kos_query_device) (uint64_t, uint64_t name);
static void* (*kos_load_device_function) (uint64_t device, const char* name);
static uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

#define CB_CREATE AQUABSD_ALPS_WM_CB_CREATE
#define CB_MODIFY AQUABSD_ALPS_WM_CB_MODIFY
#define CB_DELETE AQUABSD_ALPS_WM_CB_DELETE
#define CB_LEN    AQUABSD_ALPS_WM_CB_LEN

#define cb_t aquabsd_alps_wm_cb_t
#define wm_t aquabsd_alps_wm_t

// I cannot for the life of me figure out what the XCB equivalent of these types are
// I found all of them defined over here though: https://code.woboq.org/qt5/include/X11/Xatom.h.html

#define XA_ATOM   ((xcb_atom_t) 4)
#define XA_WINDOW ((xcb_atom_t) 33)