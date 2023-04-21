#include <umber.h>
#define UMBER_COMPONENT "aquabsd.alps.vk"

#include <aquabsd.alps.vk/private.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_cb(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT type,
	uint64_t src_obj,
	size_t location,
	int32_t code,
	char const* layer_prefix,
	char const* msg,
	void* data
) {
	(void) type;
	(void) src_obj;
	(void) location;
	(void) code;
	(void) data;

	if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		LOG_VERBOSE("@%s: %s", layer_prefix, msg)

	else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		LOG_INFO("@%s: %s", layer_prefix, msg)

	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		LOG_WARN("@%s: %s", layer_prefix, msg)

	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		LOG_WARN("@%s (PERF): %s", layer_prefix, msg)

	else if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		LOG_ERROR("@%s: %s", layer_prefix, msg)

	return false;
}

static char const* vk_error_str(VkResult err) {
	#define CASE(error) \
		case error: return #error;

	switch (err) {
		CASE(VK_ERROR_OUT_OF_HOST_MEMORY)
		CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY)
		CASE(VK_ERROR_INITIALIZATION_FAILED)
		CASE(VK_ERROR_DEVICE_LOST)
		CASE(VK_ERROR_MEMORY_MAP_FAILED)
		CASE(VK_ERROR_LAYER_NOT_PRESENT)
		CASE(VK_ERROR_EXTENSION_NOT_PRESENT)
		CASE(VK_ERROR_FEATURE_NOT_PRESENT)
		CASE(VK_ERROR_INCOMPATIBLE_DRIVER)
		CASE(VK_ERROR_TOO_MANY_OBJECTS)
		CASE(VK_ERROR_FORMAT_NOT_SUPPORTED)

		// Khronos extensions

		CASE(VK_ERROR_SURFACE_LOST_KHR)
		CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
		CASE(VK_SUBOPTIMAL_KHR)
		CASE(VK_ERROR_OUT_OF_DATE_KHR)
		CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)

		// other extensions

		CASE(VK_ERROR_VALIDATION_FAILED_EXT)

	case VK_SUCCESS:
		return "Success; this shouldn't happen, aquabsd.alps.vk has a bug";

	default:
		return "unknown Vulkan error";
	}

	#undef CASE
}

void* get_func(context_t* context, const char* name) {
	LOG_VERBOSE("%p: Get function '%s'", context, name)

	return vkGetInstanceProcAddr(context->instance, name);
}

// gotten functions are prefixed with dyn_ so they don't conflict with unused declarations in vulkan.h

#define IMPORT_FUNC(name) \
	PFN_##name const dyn_##name = (PFN_##name) vkGetInstanceProcAddr(context->instance, #name); \
	if (!dyn_##name) /* just log - leave error handling to caller */ \
		LOG_FATAL("Can't get function pointer for " #name)

void free_context(context_t* context) {
	if (context->has_device)
		vkDestroyDevice(context->device, NULL);

	if (context->has_surface)
		vkDestroySurfaceKHR(context->instance, context->surface, NULL);

	if (context->created_debug_report_cb) {
		IMPORT_FUNC(vkDestroyDebugReportCallbackEXT);

		if (dyn_vkDestroyDebugReportCallbackEXT)
			dyn_vkDestroyDebugReportCallbackEXT(context->instance, context->debug_report, NULL);
	}

	if (context->has_instance)
		vkDestroyInstance(context->instance, NULL);
}

context_t* create_win_context(
	aquabsd_alps_win_t* win,
	char const* name,
	size_t ver_major,
	size_t ver_minor,
	size_t ver_patch
) {
	VkResult rv = VK_SUCCESS;

	// TODO some LOG_VERBOSE's here and there, so that all this work isn't hidden away ;)

	context_t* const context = calloc(1, sizeof *context);

	// extension and validation layer lists
	// other possible validation layers include:
	// - VK_LAYER_LUNARG_standard_validation
	// - VK_LAYER_LUNARG_threading
	// - VK_LAYER_GOOGLE_threading
	// - VK_LAYER_LUNARG_draw_state
	// - VK_LAYER_LUNARG_image
	// - VK_LAYER_LUNARG_mem_tracker
	// - VK_LAYER_LUNARG_object_tracker
	// - VK_LAYER_LUNARG_param_checker

	char const* const validation_layers[] = {
		"VK_LAYER_KHRONOS_validation",
	};

	char const* const instance_extensions[] = {
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,
	};

	char const* const device_extensions[] = {
	};

	// create instance

	VkApplicationInfo const app = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.apiVersion = VK_MAKE_VERSION(1, 0, 2),
		.applicationVersion = VK_MAKE_VERSION(ver_major, ver_minor, ver_patch),
		.pApplicationName = name,
	};

	VkInstanceCreateInfo const instance_create = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app,
		.enabledLayerCount = sizeof(validation_layers) / sizeof(*validation_layers),
		.ppEnabledLayerNames = validation_layers,
		.enabledExtensionCount = sizeof(instance_extensions) / sizeof(*instance_extensions),
		.ppEnabledExtensionNames = instance_extensions,
	};

	rv = vkCreateInstance(&instance_create, NULL, &context->instance);

	if (rv != VK_SUCCESS) {
		LOG_FATAL("vkCreateInstance: %s", vk_error_str(rv))
		goto err;
	}

	context->has_instance = true;

	// setup debugging

	VkDebugReportCallbackCreateInfoEXT debug_report_cb_create = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		.pfnCallback = debug_cb,
		.flags =
			VK_DEBUG_REPORT_DEBUG_BIT_EXT |
			VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
			VK_DEBUG_REPORT_WARNING_BIT_EXT |
			VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
			VK_DEBUG_REPORT_ERROR_BIT_EXT,
	};

	IMPORT_FUNC(vkCreateDebugReportCallbackEXT);

	if (!dyn_vkCreateDebugReportCallbackEXT)
		goto err;

	rv = dyn_vkCreateDebugReportCallbackEXT(context->instance, &debug_report_cb_create, NULL, &context->debug_report);
	
	if (rv != VK_SUCCESS){
		LOG_FATAL("vkCreateDebugReportCallbackEXT(%p): %s", win, vk_error_str(rv))
		goto err;
	}

	context->created_debug_report_cb = true;
	
	// create surface for XCB window of passed window

	VkXcbSurfaceCreateInfoKHR surface_create = {
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.connection = win->connection,
		.window = win->win,
	};

	rv = vkCreateXcbSurfaceKHR(context->instance, &surface_create, NULL, &context->surface);

	if (rv != VK_SUCCESS) {
		LOG_FATAL("vkCreateXcbSurfaceKHR(%p): %s", win, vk_error_str(rv))
		goto err;
	}

	context->has_surface = true;

	// find an appropriate physical device
	// just use the first one for now

	uint32_t gpu_count;
	vkEnumeratePhysicalDevices(context->instance, &gpu_count, NULL);

	if (!gpu_count) {
		LOG_FATAL("No physical devices found")
		goto err;
	}

	VkPhysicalDevice* const gpus = calloc(gpu_count, sizeof *gpus);
	vkEnumeratePhysicalDevices(context->instance, &gpu_count, gpus);

	for (size_t i = 0; i < gpu_count; i++) {
		VkPhysicalDevice* const gpu = &gpus[i];

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(*gpu, &props);

		int const major = VK_VERSION_MAJOR(props.apiVersion);
		int const minor = VK_VERSION_MINOR(props.apiVersion);
		int const patch = VK_VERSION_PATCH(props.apiVersion);

		LOG_VERBOSE("Found physical device %zu: %s (%x:%x, Vulkan %d.%d.%d)", i, props.deviceName, props.vendorID, props.deviceID, major, minor, patch)
	}

	VkPhysicalDevice const gpu = gpus[0]; // XXX this can't be a reference as gpus will be freed
	free(gpus);

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(gpu, &props);

	// check if physical device has a queue family which supports graphics
	// TODO this should factor into our search for an appropriate physical device

	uint32_t family_count;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, NULL);

	VkQueueFamilyProperties* const family_props = calloc(family_count, sizeof *family_props);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, family_props);

	size_t queue_family_index;

	for (queue_family_index = 0; queue_family_index < family_count; queue_family_index++) {
		VkQueueFamilyProperties* const props = &family_props[queue_family_index];

		if (props->queueFlags & VK_QUEUE_GRAPHICS_BIT)
			goto found;
	}

	LOG_FATAL("No queue family supporting graphics found")
	goto err;

found: {}

	// check if physical device supports our surface

	VkBool32 supported = VK_FALSE;
	rv = vkGetPhysicalDeviceSurfaceSupportKHR(gpu, queue_family_index, context->surface, &supported);

	if (rv != VK_SUCCESS) {
		LOG_FATAL("vkGetPhysicalDeviceSurfaceSupportKHR: %s", vk_error_str(rv))
		goto err;
	}

	if (!supported) {
		LOG_FATAL("Physical device doesn't support window surface")
		goto err;
	}

	// create logical device

	float const queue_prios[] = { 1 };

	VkDeviceQueueCreateInfo const device_queue_create[] = {
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queue_family_index,
			.queueCount = sizeof(queue_prios) / sizeof(*queue_prios),
			.pQueuePriorities = queue_prios,
		}
	};

	VkDeviceCreateInfo const device_create = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = sizeof(device_queue_create) / sizeof(*device_queue_create),
		.pQueueCreateInfos = device_queue_create,
		.enabledExtensionCount = sizeof(device_extensions) / sizeof(*device_extensions),
		.ppEnabledExtensionNames = device_extensions,
	};

	rv = vkCreateDevice(gpu, &device_create, NULL, &context->device);

	if (rv != VK_SUCCESS) {
		LOG_FATAL("vkCreateDevice: %s", vk_error_str(rv))
		goto err;
	}

	context->has_device = true;

	return context;

err:

	free_context(context);
	return NULL;
}
