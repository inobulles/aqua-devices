#define dynamic

#include <core.fs/public.h>

#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

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
