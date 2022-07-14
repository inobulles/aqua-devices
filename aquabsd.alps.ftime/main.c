#include <aquabsd.alps.ftime/private.h>
#include <aquabsd.alps.ftime/functions.h>

typedef enum {
	TODO
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	LOG_ERROR("This device is currently for exclusive use of other devices")

	return -1;
}
