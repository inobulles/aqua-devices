#define dynamic

#include <aquabsd.alps.vga/public.h>

// these functions must be set by the first backend that manages to initialize
// 'mode_t' must be called 'video_mode_t' as it conflicts with 'mode_t' in 'sys/ipc.h' on FreeBSD unfortunately

#define video_mode_t aquabsd_alps_vga_mode_t

static int (*backend_get_mode_count) (void);
static video_mode_t* (*backend_get_modes) (void);

static int (*backend_set_mode) (video_mode_t* mode);
static void* (*backend_get_framebuffer) (void);

static int (*backend_flip) (void);

#if !defined(WITHOUT_X11)
	#include <aquabsd.alps.vga/backends/x11.h>
#endif

#if !defined(WITHOUT_AQUABSD_VGA)
	#include <aquabsd.alps.vga/backends/aquabsd_vga.h>
#endif
