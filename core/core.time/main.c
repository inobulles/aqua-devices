#include <time.h>
#include <stdint.h>

typedef enum {
	CMD_UNIX = 0x7578, // 'ux'
} cmd_t;

uint64_t send(uint16_t _cmd, __attribute__((unused)) void* data) {
	cmd_t cmd = _cmd;

	if (cmd == CMD_UNIX) {
		return time(NULL);
	}

	return -1;
}
