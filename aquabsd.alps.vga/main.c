#include <aquabsd.alps.vga/private.h>
#include <aquabsd.alps.vga/functions.h>

#define CMD_MODE_COUNT  0x6D63 // 'mc'
#define CMD_MODES       0x6D64 // 'md'

#define CMD_SET_MODE    0x736D // 'sm'
#define CMD_FRAMEBUFFER 0x6662 // 'fb'

#define CMD_FLIP        0x666C // 'fl'

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

	if (command == CMD_MODE_COUNT) {
		return get_mode_count();
	}

	else if (command == CMD_MODES) {
		return (uint64_t) get_modes();
	}

	else if (command == CMD_SET_MODE) {
		video_mode_t* mode = (video_mode_t*) arguments[0];
		return set_mode(mode);
	}

	else if (command == CMD_FRAMEBUFFER) {
		return (uint64_t) get_framebuffer();
	}

	else if (command == CMD_FLIP) {
		return flip();
	}

	return -1;
}
