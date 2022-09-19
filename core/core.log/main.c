#include <core.log/private.h>

typedef enum {
	CMD_LOG = 0x6C67, // 'lg'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_LOG) {
		umber_lvl_t lvl = args[0];

		const char* component = (void*) (intptr_t) args[1];
		const char* path      = (void*) (intptr_t) args[2];
		const char* func      = (void*) (intptr_t) args[3];

		uint32_t line = args[4];
		char* msg = (void*) (intptr_t) args[5];

		umber_log(lvl, component, path, func, line, msg);
		return 0;
	}

	return -1;
}
