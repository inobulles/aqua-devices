#include <aquabsd.alps.ogl/private.h>
#include <aquabsd.alps.ogl/functions.h>

#define CMD_CREATE       0x6363 // 'cc'
#define CMD_DELETE       0x6364 // 'dc'

#define CMD_GET_FUNCTION 0x6766 // 'gf'

int load(
    uint64_t (*_kos_query_device) (uint64_t, uint64_t name),
    void* (*_kos_load_device_function) (uint64_t device, const char* name),
    uint64_t (*_kos_callback) (uint64_t callback, int argument_count, ...)) {

    kos_query_device = _kos_query_device;
    kos_load_device_function = _kos_load_device_function;
	kos_callback = _kos_callback;

    win_device = kos_query_device(0, (uint64_t) "aquabsd.alps.win");

	if (win_device != -1) {
    	aquabsd_alps_win_register_dev_cb = kos_load_device_function(win_device, "register_dev_cb");
	}

    return 0;
}

uint64_t send(uint16_t command, void* data) {
	uint64_t* arguments = (uint64_t*) data;

	if (command == CMD_CREATE) {
		context_type_t type = arguments[0];
		context_t* context = NULL;
		
		if (type == CONTEXT_TYPE_WIN && win_device != -1) {
			aquabsd_alps_win_t* win = (void*) arguments[1];
			context = create_win_context(win);
		}

		return (uint64_t) context;
	}

	else if (command == CMD_DELETE) {
		context_t* context = (void*) arguments[0];
		return delete(context);
	}

	else if (command == CMD_GET_FUNCTION) {
		context_t* context = (void*) arguments[0];
		const char* name = (void*) arguments[1];

		return (uint64_t) get_function(context, name);
	}

	return -1;
}
