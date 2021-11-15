#include <aquabsd.alps.ogl/private.h>
#include <aquabsd.alps.ogl/functions.h>

int load(
	uint64_t (*_kos_query_device) (uint64_t, uint64_t name),
	void* (*_kos_load_device_function) (uint64_t device, const char* name),
	uint64_t (*_kos_callback) (uint64_t callback, int argument_count, ...)) {

	kos_query_device = _kos_query_device;
	kos_load_device_function = _kos_load_device_function;

	return 0;
}

uint64_t send(uint16_t command, void* data) {
	uint64_t* arguments = (uint64_t*) data;

	return -1;
}
