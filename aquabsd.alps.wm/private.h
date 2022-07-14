#define dynamic

#include <aquabsd.alps.wm/public.h>

uint64_t (*kos_query_device) (uint64_t, uint64_t name);
void* (*kos_load_device_function) (uint64_t device, const char* name);
uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

#define CB_CREATE  AQUABSD_ALPS_WM_CB_CREATE
#define CB_SHOW    AQUABSD_ALPS_WM_CB_SHOW
#define CB_HIDE    AQUABSD_ALPS_WM_CB_HIDE
#define CB_MODIFY  AQUABSD_ALPS_WM_CB_MODIFY
#define CB_DELETE  AQUABSD_ALPS_WM_CB_DELETE
#define CB_FOCUS   AQUABSD_ALPS_WM_CB_FOCUS
#define CB_STATE   AQUABSD_ALPS_WM_CB_STATE
#define CB_CAPTION AQUABSD_ALPS_WM_CB_CAPTION
#define CB_DWD     AQUABSD_ALPS_WM_CB_DWD
#define CB_CLICK   AQUABSD_ALPS_WM_CB_CLICK
#define CB_LEN     AQUABSD_ALPS_WM_CB_LEN

#define cb_t aquabsd_alps_wm_cb_t
#define provider_t aquabsd_alps_wm_provider_t
#define wm_t aquabsd_alps_wm_t

#define win_t aquabsd_alps_win_t // from aquabsd.alps.win
