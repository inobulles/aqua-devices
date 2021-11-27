#include <iar.h>

typedef enum {
	CMD_OPEN_READ         = 0x6F72, // 'or'
	CMD_CLOSE             = 0x636C, // 'cl'

	CMD_FIND_NODE         = 0x666E, // 'fn'
	CMD_READ_NODE_NAME    = 0x726E, // 'rn'
	CMD_READ_NODE_CONTENT = 0x7263, // 'rc'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_OPEN_READ) {
		iar_file_t* iar = (void*) args[0];
		const char* path = (void*) args[1];

		return iar_open_read(iar, path);
	}

	else if (cmd == CMD_CLOSE) {
		iar_file_t* iar = (void*) args[0];
		iar_close(iar);

		return 0;
	}

	else if (cmd == CMD_FIND_NODE) {
		iar_file_t* iar = (void*) args[0];
		iar_node_t* node = (void*) args[1];

		const char* name = (void*) args[2];
		iar_node_t* parent = (void*) args[3];

		return iar_find_node(iar, node, name, parent);
	}

	else if (cmd == CMD_READ_NODE_NAME) {
		iar_file_t* iar = (void*) args[0];
		iar_node_t* node = (void*) args[1];

		void* buf = (void*) args[2];

		return iar_read_node_name(iar, node, buf);
	}

	else if (cmd == CMD_READ_NODE_CONTENT) {
		iar_file_t* iar = (void*) args[0];
		iar_node_t* node = (void*) args[1];

		void* buf = (void*) args[2];

		return iar_read_node_content(iar, node, buf);
	}

	return -1;
}
