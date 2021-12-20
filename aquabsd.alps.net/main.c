#include <aquabsd.alps.net/private.h>
#include <aquabsd.alps.net/functions.h>

typedef enum {
	CMD_GET  = 0x6872, // 'gr'
	CMD_FREE = 0x6672, // 'fr'
	CMD_READ = 0x7272, // 'rr'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_GET) {
		const char* url = (void*) args[0];
		return (uint64_t) get_response(url);
	}

	else if (cmd == CMD_FREE) {
		response_t* response = (void*) args[0];
		return free_response(response);
	}

	else if (cmd == CMD_READ) {
		response_t* response = (void*) args[0];
		return (uint64_t) read_response(response);
	}

	return -1;
}
