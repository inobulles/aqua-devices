#define dynamic

#include <core.fs/public.h>

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include <umber.h>
#define UMBER_COMPONENT "core.fs"

// https://reviews.freebsd.org/D29323
// seems like in older versions of FreeBSD, the 'O_PATH' define was omitted in 'fcntl.h'

#if !defined(O_PATH)
	#define O_PATH 0x00400000
#endif

// types

#define flags_t core_fs_flags_t
#define err_t   core_fs_err_t
#define descr_t core_fs_descr_t

// flags_t

#define FLAGS_READ   CORE_FS_FLAGS_READ
#define FLAGS_WRITE  CORE_FS_FLAGS_WRITE
#define FLAGS_EXEC   CORE_FS_FLAGS_EXEC
#define FLAGS_APPEND CORE_FS_FLAGS_APPEND

#define FLAGS_CREATE CORE_FS_FLAGS_CREATE
#define FLAGS_STREAM CORE_FS_FLAGS_STREAM

// err_t

#define ERR_SUCCESS CORE_FS_ERR_SUCCESS
#define ERR_GENERIC CORE_FS_ERR_GENERIC
