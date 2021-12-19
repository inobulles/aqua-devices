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

static int read_apps(char*** list_ref) {
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
		if (!validate_pkg(app)) {
			continue;
		}

		count += 1;
		
		if (!list_ref) {
			continue; // don't go any further if this is just a dryrun
		}

		char* path = malloc(strlen(APPS_PATH) + strlen(app->d_name) + 2 /* strlen("/") + 1 */);
		sprintf(path, "%s/%s", APPS_PATH, app->d_name);

		*list_ref = realloc(*list_ref, count * sizeof **list_ref);
		(*list_ref)[count - 1] = path; // apparently indexing has a higher precedence than deferencing 🤷
	}

	closedir(dp);
	return count;
}

int app_count(void) {
	return read_apps(NULL);
}

char** app_list(void) {
	char** list = NULL;
	
	if (read_apps(&list) < 0) {
		return NULL;
	}

	return list;
}
