#if !defined(__CORE_FS)
#define __CORE_FS

#include <stddef.h>

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
	size_t bytes;

	int mmap_flags;
	void* mem;
} core_fs_descr_t;

#endif