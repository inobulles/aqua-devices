#include <core.fs/functions.h>

typedef enum {
	CMD_OPEN  = 0x6F70, // 'op'
	CMD_CLOSE = 0x636C, // 'cl'

	CMD_SIZE  = 0x737A, // 'sz'
	CMD_MMAP  = 0x6D6D, // 'mm'

	CMD_READ  = 0x7264, // 'rd'
	CMD_WRITE = 0x7772, // 'wr'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	// access commands

	if (cmd == CMD_OPEN) {
		const char* drive = (const void*) (intptr_t) args[0];
		const char* path  = (const void*) (intptr_t) args[1];

		flags_t flags = args[2];

		return (uint64_t) (intptr_t) fs_open(drive, path, flags);
	}

	else if (cmd == CMD_CLOSE) {
		descr_t* descr = (void*) (intptr_t) args[0];
		return fs_close(descr);
	}

	// info commands

	else if (cmd == CMD_SIZE) {
		descr_t* descr = (void*) (intptr_t) args[0];
		return (uint64_t) (intptr_t) fs_size(descr);
	}

	else if (cmd == CMD_MMAP) {
		descr_t* descr = (void*) (intptr_t) args[0];
		return (uint64_t) (intptr_t) fs_mmap(descr);
	}

	// stream commands

	else if (cmd == CMD_READ) {
		descr_t* descr = (void*) (intptr_t) args[0];
		void* buf = (void*) (intptr_t) args[1];
		size_t len = args[2];

		return fs_read(descr, buf, len);
	}

	else if (cmd == CMD_WRITE) {
		descr_t* descr = (void*) (intptr_t) args[0];
		const void* buf = (const void*) (intptr_t) args[1];
		size_t len = args[2];

		return fs_write(descr, buf, len);
	}

	return -1;
}
