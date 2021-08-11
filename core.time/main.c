#include <time.h>
#include <stdint.h>

#define CMD_UNIX 0x7578 // 'ux'

uint64_t send(uint16_t command, void* data) {
	if (command == CMD_UNIX) {
		return time(NULL);
	}

	return -1;
}
