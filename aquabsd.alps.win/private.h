#define dynamic

#include <aquabsd.alps.win/public.h>

static uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

#define cb_t aquabsd_alps_win_cb_t

#define CB_DRAW AQUABSD_ALPS_WIN_CB_DRAW
#define CB_LEN AQUABSD_ALPS_WIN_CB_LEN

#define win_t aquabsd_alps_win_t
