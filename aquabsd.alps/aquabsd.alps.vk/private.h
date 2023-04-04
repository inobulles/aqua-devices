#define dynamic

#include <aquabsd.alps.vk/public.h>

uint64_t (*kos_query_device) (uint64_t, uint64_t name);
void* (*kos_load_device_function) (uint64_t device, const char* name);
uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

static uint64_t win_device = -1;
static uint64_t ftime_device = -1;

#define context_type_t aquabsd_alps_vk_context_type_t

#define CONTEXT_TYPE_WIN AQUABSD_ALPS_VULKAN_CONTEXT_TYPE_WIN
#define CONTEXT_TYPE_WM  AQUABSD_ALPS_VULKAN_CONTEXT_TYPE_WM
#define CONTEXT_TYPE_LEN AQUABSD_ALPS_VULKAN_CONTEXT_TYPE_LEN

#define context_t aquabsd_alps_vk_context_t
