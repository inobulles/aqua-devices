#include <aquabsd.alps.ftime/private.h>
#include <aquabsd.alps.ftime/functions.h>

typedef enum {
	CMD_CREATE = 0x6674, // 'ft'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_CREATE) {
		double target = *(double*) &args[0];
		return (uint64_t) create(target);
	}

	return -1;
}
