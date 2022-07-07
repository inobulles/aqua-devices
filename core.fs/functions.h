#include <core.fs/private.h>

// access commands

dynamic descr_t* fs_open(const char* drive, const char* path, flags_t flags) {
	descr_t* rv = (void*) ERR_GENERIC;

	// flags for later syscalls

	int o_flags = 0;
	int mmap_flags = 0;

	// do we wanna read or write?

	if (
		flags & FLAGS_READ &&
		flags & FLAGS_WRITE
	) {
		o_flags |= O_RDWR;
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

	// TODO check permissions here

	// chdir into the drive we want to access

	bool sys = !strcmp(drive, "sys");
	char* dir = NULL;

	if (sys) {
		dir = "/";
	}

	else if (!strcmp(drive, "dat")) {
		dir = "TODO";
	}

	else if (!strcmp(drive, "wrk")) {
		// don't go anywhere; we're already where we need to be
	}

	else {
		LOG_VERBOSE("Couldn't identify drive of '%s:%s'", drive, path)
		goto error;
	}

	char* cwd = getcwd(NULL, 0);

	if (!dir) {
		dir = cwd;
	}

	else if (chdir(dir) < 0) {
		LOG_VERBOSE("Couldn't change into drive directory of '%s:%s' ('%s')", drive, path, dir)
		goto chdir_error;
	}

	// open the file now
	// if it doesn't yet exist, the 'realpath' call will fail (which we don't want if 'flags & FLAGS_CREATE')
	// we need to specify a mode for when 'o_flags & O_CREAT'

	mode_t mode =
		S_IRUSR | S_IWUSR | // owner can read/write
		S_IRGRP | S_IWGRP;  // group can read/write

	int fd = open(path, o_flags, mode);

	if (fd < 0) {
		LOG_VERBOSE("open(\"%s:%s\"): %s", drive, path, strerror(errno))
		goto open_error;
	}

	// check if path is well a subdirectory of the drive path

	char* abs_path = realpath(path, NULL);

	if (!abs_path) {
		LOG_VERBOSE("realpath(\"%s:%s\") (from within \"%s\"): %s", drive, path, dir, strerror(errno))
	}

	if (dir != cwd && chdir(cwd) < 0) {
		LOG_ERROR("chdir(\"%s\"): %s (potential race condition?)", cwd, strerror(errno))
	}

	if (!abs_path || strncmp(abs_path, dir, strlen(dir))) {
		LOG_VERBOSE("\"%s:%s\" is not contained within \"%s\" (there may be malicious intent at play here!)", drive, path, dir)

		close(fd);
		goto hierarchy_error;
	}

	// stat file and map to memory

	struct stat st;

	if (fstat(fd, &st) < 0) {
		LOG_VERBOSE("fstat(\"%s:%s\"): %s", drive, path, strerror(errno));

		close(fd);
		goto stat_error;
	}

	void* mem = NULL;

	if (flags & FLAGS_STREAM) {
		mem = mmap(NULL, st.st_size, mmap_flags, MAP_SHARED, fd, 0);

		if (!mem) {
			LOG_VERBOSE("mmap(\"%s:%s\", %zu): %s", drive, path, st.st_size, strerror(errno));

			close(fd);
			goto mmap_error;
		}
	}

	// create descriptor structure ('descr_t')

	descr_t* descr = calloc(1, sizeof *descr);

	descr->fd = fd;
	descr->mem = mem;
	descr->bytes = st.st_size;

	// success

	rv = descr;

mmap_error:
stat_error:
hierarchy_error:

	free(abs_path);

open_error:
chdir_error:

	free(cwd);

error:

	return rv;
}

dynamic err_t fs_close(descr_t* descr) {
	if (descr->mem) {
		munmap(descr->mem, descr->bytes);
	}

	close(descr->fd);
	free(descr);

	return ERR_SUCCESS;
}

// stream commands

dynamic err_t fs_read(descr_t* descr, void* buf, size_t len) {
	if (!descr) {
		return ERR_GENERIC;
	}

	if (read(descr->fd, buf, len) < 0) {
		return ERR_GENERIC;
	}

	return ERR_SUCCESS;
}

dynamic err_t fs_write(descr_t* descr, const void* buf, size_t len) {
	if (!descr) {
		return ERR_GENERIC;
	}

	if (write(descr->fd, buf, len) < 0) {
		return ERR_GENERIC;
	}

	return ERR_SUCCESS;
}