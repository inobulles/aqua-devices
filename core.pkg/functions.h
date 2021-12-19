// TODO include literally comes verbatim from the KOS source
//      this should be a single file and there should be a way for devices to include headers from the KOS

#include <core.pkg/pkg_t.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>
#include <dirent.h>

#define ERROR(...) fprintf(stderr, "[core.pkg] ERROR " __VA_ARGS__);

#define APPS_PATH "apps"

static int validate_pkg(struct dirent* pkg) {
	// go through a bunch of checks to be sure our package is valid

	if (pkg->d_type != DT_REG) {
		return 0; // anything other than a regular file is invalid
	}

	char* extension = strrchr(pkg->d_name, '.');

	if (!extension) {
		return 0; // package needs an extension
	}

	if (strcmp(extension, ".zpk")) {
		return 0; // only ZPK packages are supported currently
	}

	return 1;
}

int app_count(void) {
	// make sure an APPS_PATH directory

	if (mkdir(APPS_PATH, 0700) < 0 && errno != EEXIST) {
		ERROR("Failed to create apps directory (" APPS_PATH ") in root directory\n")
		return -1;
	}

	DIR* dp = opendir(APPS_PATH);

	if (!dp) {
		ERROR("Failed to open apps directory (" APPS_PATH ") in root directory\n")
		return -1;
	}

	unsigned count = 0;
	struct dirent* app;

	while ((app = readdir(dp))) {
		count += validate_pkg(app);
	}

	closedir(dp);
	return count;
}

char** app_list(void) {
	return NULL;
}
