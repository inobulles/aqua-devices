#define __STDC_WANT_LIB_EXT2__ 1 // ISO/IEC TR 24731-2:2010 standard library extensions

// TODO include literally comes verbatim from the KOS source
//      this should be a single file and there should be a way for devices to include headers from the KOS

#include <core.pkg/pkg_t.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h>
#include <sys/stat.h>

#include <umber.h>
#define UMBER_COMPONENT "core.pkg"

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
		LOG_ERROR("Failed to create apps directory (" APPS_PATH ") in root directory")
		return -1;
	}

	DIR* dp = opendir(APPS_PATH);

	if (!dp) {
		LOG_ERROR("Failed to open apps directory (" APPS_PATH ") in root directory")
		return -1;
	}

	size_t count = 0;
	struct dirent* app;

	while ((app = readdir(dp))) {
		if (!validate_pkg(app)) {
			continue;
		}

		count += 1;

		if (!list_ref) {
			continue; // don't go any further if this is just a dryrun
		}

		char* path;
		if (asprintf(&path, APPS_PATH "/%s", app->d_name));

		*list_ref = realloc(*list_ref, count * sizeof **list_ref);
		(*list_ref)[count - 1] = path; // apparently indexing has a higher precedence than dereferencing 🤷
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
