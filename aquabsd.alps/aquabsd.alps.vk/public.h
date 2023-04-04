#if !defined(__AQUABSD_ALPS_VK)
#define __AQUABSD_ALPS_VK

#include <aquabsd.alps.ftime/public.h>
#include <aquabsd.alps.win/public.h>

#include <vulkan/vulkan.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef enum {
	AQUABSD_ALPS_VK_CONTEXT_TYPE_WIN,
	AQUABSD_ALPS_VK_CONTEXT_TYPE_LEN,
} aquabsd_alps_vk_context_type_t;

typedef struct {
	unsigned x_res, y_res;

	// Vulkan stuff

	VkDebugReportCallbackEXT debug_report;

	bool has_instance;
	VkInstance instance;

	bool created_debug_report_cb;

	// backend stuff

	aquabsd_alps_vk_context_type_t backend_type;

	aquabsd_alps_win_t* win;
	xcb_window_t draw_win;

	// frame timing stuff

	double target_ftime;
	aquabsd_alps_ftime_t* ftime;

	// EGL stuff

	EGLDisplay* egl_display;
	EGLContext* egl_context;
	EGLSurface* egl_surface;
} aquabsd_alps_vk_context_t;

aquabsd_alps_vk_context_t* (*aquabsd_alps_vk_create_win_context) (aquabsd_alps_win_t* win);
int (*aquabsd_alps_vk_delete_context) (aquabsd_alps_vk_context_t* context);

void* (*aquabsd_alps_vk_get_function) (aquabsd_alps_vk_context_t* context, const char* name);
int (*aquabsd_alps_vk_bind_win_tex) (aquabsd_alps_vk_context_t* context, aquabsd_alps_win_t* win);

#endif
