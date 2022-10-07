#include <core.log/private.h>

typedef enum {
	CMD_LOG = 0x6C67, // 'lg'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t const cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_LOG) {
		umber_lvl_t const lvl = args[0];

		char const* const component = (void*) (intptr_t) args[1];
		char const* const path      = (void*) (intptr_t) args[2];
		char const* const func      = (void*) (intptr_t) args[3];

		uint32_t const line = args[4];
		char const* const msg = (void*) (intptr_t) args[5];

		umber_log(lvl, component, path, func, line, msg);
		return 0;
	}

	return -1;
}
