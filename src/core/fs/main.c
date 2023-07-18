// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

// TODO see how things behave when taking multiple threads into account
//      there may be a need for some mutices around the temporary chdir for realpath
//      the real solution is probably to just not chdir actually

#include <core/fs/private.h>
#include <core/pkg/pkg_t.h>

// some KOS-set variables which are interesting to us

char* conf_path;
char* unique_path;

void* (*pkg_read) (pkg_t* pkg, char const* key, iar_node_t* parent, uint64_t* bytes_ref);
pkg_t* boot_pkg;

// helpful macros

#define VALIDATE_DESCR(rv) do { \
	if (!descr) { \
		LOG_WARN("Descriptor pointer is NULL") \
		return (rv); \
	} \
} while (0)

static void strfree(char** str_ref) {
	if (*str_ref) {
		free(*str_ref);
	}
}

// access commands

static descr_t* res_open(char const* path, flags_t flags) {
	// make sure the flags make sense
	// we can't write to the resources drive!

	flags_t const illegal_flags = FLAGS_WRITE | FLAGS_APPEND | FLAGS_CREATE;

	if (flags & illegal_flags) {
		LOG_ERROR("Passed flags (0x%x) are illegal on the resource drive (matches 0x%x)", flags, illegal_flags)
		return NULL;
	}

	// build key

	char* __attribute__((cleanup(strfree))) key = NULL;
	if (asprintf(&key, "res/%s", path)) {}

	// read resource

	uint64_t bytes;
	void* const mem = pkg_read(boot_pkg, "res/main.wgsl", NULL, &bytes);

	if (mem == NULL) {
		LOG_ERROR("pkg_read(\"%s\"): Failed to read key on boot package", key)
		return NULL;
	}

	descr_t* const descr = calloc(1, sizeof *descr);

	descr->drive = strdup("res");
	descr->res_drive = true;

	descr->path = strdup(path);
	descr->flags = flags;

	descr->fd = -1;
	descr->size = bytes;
	descr->mem = mem;

	return descr;
}

descr_t* fs_open(char const* drive, char const* path, flags_t flags) {
	descr_t* rv = NULL;

	// TODO check permissions here, when we implement permissions

	// identify drive

	if (strcmp(drive, "res") == 0) {
		return res_open(path, flags);
	}

	char* dir = NULL;

	if (strcmp(drive, "sys") == 0) {
		dir = "/";
	}

	else if (strcmp(drive, "cfg") == 0) {
		dir = conf_path;
	}

	else if (strcmp(drive, "wrk") == 0) {
		// don't go anywhere; we're already where we need to be
	}

	else if (strcmp(drive, "unq") == 0) {
		dir = unique_path;
	}

	else {
		LOG_WARN("Couldn't identify drive of '%s:%s'", drive, path)
		goto error;
	}

	// flags for open(2) & mmap(2) syscalls later

	int o_flags = 0;
	int mmap_flags = 0;

	// do we wanna read or write?

	if (
		flags & FLAGS_READ &&
		flags & FLAGS_WRITE
	) {
		o_flags |= O_RDWR;
		mmap_flags |= PROT_READ | PROT_WRITE;
	}

	else if (flags & FLAGS_READ) {
		o_flags |= O_RDONLY;
		mmap_flags |= PROT_READ;
	}

	else if (flags & FLAGS_WRITE) {
		o_flags |= O_WRONLY;
		mmap_flags |= PROT_WRITE;
	}

	// if writing, do we want to overwrite or append?

	if (flags & FLAGS_APPEND) {
		o_flags |= O_APPEND;
	}

	// if writing, do we want to create the file if it already exists?

	if (flags & FLAGS_CREATE) {
		o_flags |= O_CREAT;
	}

	// change into directory of file

	char* const cwd = getcwd(NULL, 0);

	if (!dir) {
		dir = cwd;
	}

	else if (chdir(dir) < 0) {
		LOG_WARN("Couldn't change into drive directory of '%s:%s' ('%s')", drive, path, dir)
		goto chdir_error;
	}

	// open the file now
	// if it doesn't yet exist, the 'realpath' call will fail (which we don't want if 'flags & FLAGS_CREATE')
	// we need to specify a mode for when 'o_flags & O_CREAT'

	mode_t const mode =
		S_IRUSR | S_IWUSR | // owner can read/write
		S_IRGRP | S_IWGRP;  // group can read/write

	int const fd = open(path, o_flags, mode);

	if (fd < 0) {
		LOG_VERBOSE("open(\"%s:%s\"): %s", drive, path, strerror(errno))

		if (chdir(cwd) < 0) {
			LOG_ERROR("chdir(\"%s\"): %s (potential race condition?)", cwd, strerror(errno))
		}

		goto open_error;
	}

	// check if path is well a subdirectory of the drive path
	// although it's cumbersome, I really wanna use realpath here to reduce points of failure
	// to be honest, I think it's a mistake not to have included a proper way of checking path hierarchy in POSIX

	char* const abs_path = realpath(path, NULL);

	if (dir != cwd && chdir(cwd) < 0) {
		LOG_ERROR("chdir(\"%s\"): %s (potential race condition?)", cwd, strerror(errno))
	}

	if (!abs_path) {
		LOG_VERBOSE("realpath(\"%s:%s\") (from within \"%s\"): %s", drive, path, dir, strerror(errno))

		close(fd);
		goto realpath_path_error;
	}

	char* const abs_dir = realpath(dir, NULL);

	if (!abs_dir) {
		LOG_VERBOSE("realpath(\"%s\"): %s", dir, strerror(errno))

		close(fd);
		goto realpath_dir_error;
	}

	if (!abs_path || strncmp(abs_path, abs_dir, strlen(abs_dir))) {
		LOG_VERBOSE("\"%s:%s\" is not contained within \"%s\" (there may be malicious intent at play here!)", drive, path, dir)

		close(fd);
		goto hierarchy_error;
	}

	// stat file incase we want to mmap later

	struct stat st;

	if (fstat(fd, &st) < 0) {
		LOG_WARN("fstat(\"%s:%s\"): %s", drive, path, strerror(errno))

		close(fd);
		goto stat_error;
	}

	// create descriptor structure ('descr_t')

	descr_t* const descr = calloc(1, sizeof *descr);

	descr->drive = strdup(drive);
	descr->path = strdup(path);
	descr->flags = flags;

	descr->fd = fd;
	descr->size = st.st_size;

	descr->mmap_flags = mmap_flags;

	// success

	rv = descr;

stat_error:
hierarchy_error:

	free(abs_dir);

realpath_dir_error:

	free(abs_path);

realpath_path_error:
open_error:
chdir_error:

	free(cwd);

error:

	return rv;
}

err_t fs_close(descr_t* descr) {
	VALIDATE_DESCR(ERR_GENERIC);

	free(descr->drive);
	free(descr->path);

	if (descr->mem) {
		if (descr->res_drive) {
			free(descr->mem);
		}

		else {
			munmap(descr->mem, descr->size);
		}
	}

	if (descr->fd >= 0) {
		close(descr->fd);
	}

	free(descr);

	return ERR_SUCCESS;
}

// info commands

ssize_t fs_size(descr_t* descr) {
	VALIDATE_DESCR((ssize_t) ERR_GENERIC);
	return descr->size;
}

void* fs_mmap(descr_t* descr) {
	VALIDATE_DESCR(NULL);

	if (descr->mem) {
		return descr->mem;
	}

	descr->mem = mmap(NULL, descr->size, descr->mmap_flags, MAP_SHARED, descr->fd, 0);

	if (descr->mem == MAP_FAILED) {
		LOG_WARN("mmap(\"%s:%s\", %zu): %s", descr->drive, descr->path, descr->size, strerror(errno))
		return NULL;
	}

	return descr->mem;
}

// stream commands

err_t fs_read(descr_t* descr, void* buf, size_t len) {
	VALIDATE_DESCR(ERR_GENERIC);

	if (descr->res_drive) {
		LOG_WARN("Can't use the read command on files on the resource drive - use the mmap command instead")
		return ERR_GENERIC;
	}

	if (read(descr->fd, buf, len) < 0) {
		LOG_WARN("read(%p, %zu): %s", descr, len, strerror(errno))
		return ERR_GENERIC;
	}

	return ERR_SUCCESS;
}

err_t fs_write(descr_t* descr, void const* buf, size_t len) {
	VALIDATE_DESCR(ERR_GENERIC);

	if (descr->res_drive) {
		LOG_WARN("Can't use the write command on files on the resource drive")
		return ERR_GENERIC;
	}

	if (write(descr->fd, buf, len) < 0) {
		LOG_WARN("write(%p, %zu): %s", descr, len, strerror(errno))
		return ERR_GENERIC;
	}

	return ERR_SUCCESS;
}

typedef enum {
	CMD_OPEN  = 0x6F70, // 'op'
	CMD_CLOSE = 0x636C, // 'cl'

	CMD_SIZE  = 0x737A, // 'sz'
	CMD_MMAP  = 0x6D6D, // 'mm'

	CMD_READ  = 0x7264, // 'rd'
	CMD_WRITE = 0x7772, // 'wr'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t const cmd = _cmd;
	uint64_t* const args = data;

	// access commands

	if (cmd == CMD_OPEN) {
		char const* const drive = (void const*) (intptr_t) args[0];
		char const* const path  = (void const*) (intptr_t) args[1];

		flags_t const flags = args[2];

		return (uint64_t) (intptr_t) fs_open(drive, path, flags);
	}

	else if (cmd == CMD_CLOSE) {
		descr_t* const descr = (void*) (intptr_t) args[0];
		return fs_close(descr);
	}

	// info commands

	else if (cmd == CMD_SIZE) {
		descr_t* const descr = (void*) (intptr_t) args[0];
		return (uint64_t) (intptr_t) fs_size(descr);
	}

	else if (cmd == CMD_MMAP) {
		descr_t* const descr = (void*) (intptr_t) args[0];
		return (uint64_t) (intptr_t) fs_mmap(descr);
	}

	// stream commands

	else if (cmd == CMD_READ) {
		descr_t* const descr = (void*) (intptr_t) args[0];
		void* const buf = (void*) (intptr_t) args[1];
		size_t const len = args[2];

		return fs_read(descr, buf, len);
	}

	else if (cmd == CMD_WRITE) {
		descr_t* const descr = (void*) (intptr_t) args[0];
		void const* const buf = (void const*) (intptr_t) args[1];
		size_t const len = args[2];

		return fs_write(descr, buf, len);
	}

	return -1;
}
