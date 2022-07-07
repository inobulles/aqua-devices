#include <core.fs/private.h>

// access commands

dynamic descr_t* fs_open(const char* drive, const char* path, flags_t flags) {
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

	// TODO check permissions

	// open and map to memory

	int fd = open(path, o_flags);

	if (fd < 0) {
		return (descr_t*) ERR_GENERIC;
	}

	struct stat st;

	if (fstat(fd, &st) < 0) {
		return (descr_t*) ERR_GENERIC;
	}

	void* mem = NULL;

	if (flags & FLAGS_STREAM) {
		mem = mmap(NULL, st.st_size, mmap_flags, MAP_SHARED, fd, 0);

		if (!mem) {
			return (descr_t*) ERR_GENERIC;
		}
	}

	// create descriptor structure ('descr_t')

	descr_t* descr = calloc(1, sizeof *descr);

	descr->fd = fd;
	descr->mem = mem;
	descr->bytes = st.st_size;

	return descr;
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