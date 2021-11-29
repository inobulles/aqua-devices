#include <aquabsd.alps.wm/private.h>
#include <aquabsd.alps.wm/functions.h>

typedef enum {
	CMD_CREATE = 0x6377, // 'cw'
	CMD_DELETE = 0x6477, // 'dw'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_CREATE) {
		const char* name = (void*) args[0];
		return (uint64_t) create(name);
	}

	else if (cmd == CMD_DELETE) {
		wm_t* wm = (void*) args[0];
		return delete(wm);
	}
	
	return -1;
}
