// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#pragma once

#include <stddef.h>
#include <sys/types.h> // for some reason, 'ssize_t' is not in 'stddef.h'

typedef enum {
	CORE_FS_FLAGS_READ   = 0b00001, // O_RDONLY/O_RDWR, PROT_READ
	CORE_FS_FLAGS_WRITE  = 0b00010, // O_WRONLY/O_RDWR, PROT_WRITE
	CORE_FS_FLAGS_EXEC   = 0b00100, // PROT_EXEC
	CORE_FS_FLAGS_APPEND = 0b01000, // O_APPEND
	CORE_FS_FLAGS_CREATE = 0b10000, // O_CREAT: create file/directory if it doesn't yet exist (TODO: vs O_EXCL?)
} core_fs_flags_t;

// TODO better error handling

typedef enum {
	CORE_FS_ERR_SUCCESS = 0,
	CORE_FS_ERR_GENERIC = -1,
} core_fs_err_t;

typedef struct {
	char* drive;
	char* path;
	core_fs_flags_t flags;

	int fd;
	size_t size;

	int mmap_flags;
	void* mem;
} core_fs_descr_t;

core_fs_descr_t* (*core_fs_open)  (const char* drive, const char* path, core_fs_flags_t flags);
core_fs_err_t    (*core_fs_close) (core_fs_descr_t* descr);
ssize_t          (*core_fs_size)  (core_fs_descr_t* descr);
void*            (*core_fs_mmap)  (core_fs_descr_t* descr);
core_fs_err_t    (*core_fs_read)  (core_fs_descr_t* descr, void* buf, size_t len);
core_fs_err_t    (*core_fs_write) (core_fs_descr_t* descr, const void* buf, size_t len);
