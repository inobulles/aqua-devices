#include <aquabsd.alps.ftime/private.h>

typedef enum {
	TODO
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	__attribute__((unused)) cmd_t cmd = _cmd;
	__attribute__((unused)) uint64_t* args = data;

	return -1;
}
