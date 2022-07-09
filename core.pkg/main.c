#include <stdint.h>
#include <core.pkg/functions.h>

pkg_t* (*create_pkg) (const char* path);
void (*free_pkg) (pkg_t* pkg);

void* (*pkg_read) (pkg_t* pkg, const char* key, iar_node_t* parent, uint64_t* bytes_ref);
int (*pkg_boot) (pkg_t* pkg);

typedef enum {
	CMD_APP_COUNT = 0x6163, // 'ac'
	CMD_APP_LIST  = 0x6C73, // 'ls'

	CMD_LOAD      = 0x6C70, // 'lp'
	CMD_FREE      = 0x6670, // 'fp'

	CMD_NAME      = 0x676E, // 'gn'
	CMD_UNIQUE    = 0x6775, // 'gu'
	CMD_GENERIC   = 0x6767, // 'gg'

	CMD_BOOT      = 0x626F, // 'bo'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	// app listing commands

	if (cmd == CMD_APP_COUNT) {
		return app_count();
	}

	else if (cmd == CMD_APP_LIST) {
		return (uint64_t) app_list();
	}

	// package manipulation commands

	else if (cmd == CMD_LOAD) {
		const char* path = (void*) args[0];
		return (uint64_t) create_pkg(path);
	}

	else if (cmd == CMD_FREE) {
		pkg_t* pkg = (void*) args[0];

		free_pkg(pkg);
		return 0;
	}

	else if (cmd == CMD_NAME) {
		pkg_t* pkg = (void*) args[0];
		return (uint64_t) pkg->name;
	}

	else if (cmd == CMD_UNIQUE) {
		pkg_t* pkg = (void*) args[0];
		return (uint64_t) pkg->unique;
	}

	else if (cmd == CMD_GENERIC) {
		pkg_t* pkg = (void*) args[0];
		const char* key = (void*) args[1];

		return (uint64_t) pkg_read(pkg, key, NULL, NULL);
	}

	else if (cmd == CMD_BOOT) {
		pkg_t* pkg = (void*) args[0];
		return pkg_boot(pkg);
	}

	return -1;
}
