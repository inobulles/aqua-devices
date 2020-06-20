
#include "iar.h"
iar_file_t* boot_package_pointer = (iar_file_t*) 0;

char** unique_pointer = (char**) 0;
char** root_path_pointer = (char**) 0;
char** cwd_path_pointer = (char**) 0;

#define FS_DRIVE_ASSETS 'A'
#define FS_DRIVE_DATA   'D'
#define FS_DRIVE_CONF   'C'
#define FS_DRIVE_USER   'U'
#define FS_DRIVE_ROOT   'R'
#define FS_DRIVE_SYSTEM 'Y'

static char* conf_drive_path;
static char* user_drive_path;
static char* root_drive_path;

static size_t conf_drive_path_bytes;
static size_t user_drive_path_bytes;
static size_t root_drive_path_bytes;

void load(void) {
	chdir(*root_path_pointer);
	root_drive_path = realpath(".", (char*) 0);
	
	mkdir("conf", 0700);
	mkdir("user", 0700);

	conf_drive_path = realpath("conf", (char*) 0);
	user_drive_path = realpath("user", (char*) 0);

	conf_drive_path_bytes = strlen(conf_drive_path);
	user_drive_path_bytes = strlen(user_drive_path);
	root_drive_path_bytes = strlen(root_drive_path);
	
	chdir(*cwd_path_pointer);
}

void quit(void) {
	free(conf_drive_path);
	free(user_drive_path);
	free(root_drive_path);
}

typedef struct {
	char* raw_path;
	iar_node_t assets_node;
	
	int list_index;
	char** list;
	
	char drive;
	char path[PATH_MAX + 1];
	
	uint8_t write_access;
	uint8_t read_access;
} path_t;

static void free_path(path_t* self) {
	free(self->list);
}

static int setup_path(path_t* self, const char* raw_path) { /// TODO All the permissions
	self->raw_path = (char*) raw_path;
	self->raw_path[PATH_MAX + 1] = 0; // make sure we don't get any buffer overflows
	
	self->write_access = 0;
	self->read_access = 0;
	
	self->drive = FS_DRIVE_DATA;
	
	if (self->raw_path[1] == ':') { // has drive prefix
		self->drive = *self->raw_path;
		self->raw_path += 2;
	}
	
	switch (self->drive) {
		case FS_DRIVE_ASSETS: {
			self->read_access = 1;
			return 0;
			
		} case FS_DRIVE_DATA: {
			self->write_access = 1;
			self->read_access = 1;
			
			return 0;
			
		} case FS_DRIVE_CONF: {
			self->read_access = 1;
			if (0) self->write_access = 1; // has write permissions?
			
			return 0;
			
		} case FS_DRIVE_USER: {
			if (0) self->write_access = 0; // has write permissions?
			if (0) self->read_access = 0; // has read permissions?
			
			return 0;
			
		} case FS_DRIVE_ROOT: {
			if (0) self->write_access = 0; // has write permissions?
			if (0) self->read_access = 0; // has read permissions?
			
			return 0;
			
		} case FS_DRIVE_SYSTEM: {
			if (0) self->write_access = 0; // has write permissions?
			if (0) self->read_access = 0; // has read permissions?
			
			return 0;
			
		} default: {
			printf("WARNING Unknown drive '%c'; access denied\n", self->drive);
			return 1;
		}
	}
	
	printf("WARNING Unknown error in filtering paths; access denied\n");
	return 1;
}

static int create_path(path_t* self) {
	char temp_path[PATH_MAX + 2 /* +2 instead of +1 to fix warning message */];
	
	switch (self->drive) {
		case FS_DRIVE_ASSETS: {
			if (!boot_package_pointer) {
				printf("WARNING No IAR file found; access denied\n");
				return 1;
			}
			
			if (iar_find_node(boot_package_pointer, &self->assets_node, "assets", &boot_package_pointer->root_node) == -1) {
				printf("WARNING No assets node found; access denied\n");
				return 1;
			}
			
			int forwards = 0;
			int backwards = 0;
			
			*self->path = 0;
			uint8_t out_of_bounds = 0;
			
			self->list = (char**) malloc(sizeof(char*));
			self->list_index = 0;
			
			for (char* i = strtok(self->raw_path, "/"); i; i = strtok((char*) 0, "/")) {
				if (strcmp(i, "..") == 0) {
					if (self->list_index <= 0) {
						out_of_bounds = 1;
						break;
					}
					
					self->list = (char**) realloc(self->list, self->list_index-- * sizeof(char*));
					backwards++;
					
				} else if (strcmp(i, ".") == 0) {
					
				} else if (*i) {
					forwards++;
					
					self->list[self->list_index] = i;
					self->list = (char**) realloc(self->list, (++self->list_index + 1) * sizeof(char*));
				}
				
				out_of_bounds |= backwards > forwards;
			}
			
			if (!out_of_bounds) { // in bounds
				return 0;
			}
			
			printf("WARNING Path out of bounds; access denied\n");
			return 1;
			
		} case FS_DRIVE_DATA: {
			self->write_access = 1;
			self->read_access = 1;
			
			if (!*unique_pointer) { // no unique
				printf("WARNING Unique not found; access denied\n");
				return 1;
			}
			
			sprintf(self->path, "data/%s", *unique_pointer);
			char* parent = realpath(self->path, (char*) 0);
			
			sprintf(temp_path, "%s/%s", self->path, self->raw_path);
			realpath(temp_path, self->path);
			
			size_t child_bytes = strlen(self->path);
			size_t parent_bytes = strlen(parent);
			
			if (strncmp(parent, self->path, parent_bytes) == 0 && (child_bytes == parent_bytes || self->path[parent_bytes] == '/')) {
				free(parent);
				return 0;
			}
			
			free(parent);
			printf("WARNING Path out of bounds; access denied\n");
			return 1;
			
		} case FS_DRIVE_CONF: {
			self->read_access = 1;
			if (0) self->write_access = 1; // has write permissions?
			
			sprintf(temp_path, "conf/%s", self->raw_path);
			realpath(temp_path, self->path);
			
			size_t child_bytes = strlen(self->path);
			if (strncmp(conf_drive_path, self->path, conf_drive_path_bytes) == 0 && (child_bytes == conf_drive_path_bytes || self->path[conf_drive_path_bytes] == '/')) {
				return 0;
			}
			
			printf("WARNING Path out of bounds; access denied\n");
			return 1;
			
		} case FS_DRIVE_USER: {
			if (0) self->write_access = 0; // has write permissions?
			if (0) self->read_access = 0; // has read permissions?
			
			sprintf(temp_path, "user/%s", self->raw_path);
			realpath(temp_path, self->path);
			
			size_t child_bytes = strlen(self->path);
			if (strncmp(user_drive_path, self->path, user_drive_path_bytes) == 0 && (child_bytes == user_drive_path_bytes || self->path[user_drive_path_bytes] == '/')) {
				return 0;
			}
			
			printf("WARNING Path out of bounds; access denied\n");
			return 1;
			
		} case FS_DRIVE_ROOT: {
			if (0) self->write_access = 0; // has write permissions?
			if (0) self->read_access = 0; // has read permissions?
			
			realpath(self->raw_path, self->path);
			
			size_t child_bytes = strlen(self->path);
			if (strncmp(root_drive_path, self->path, root_drive_path_bytes) == 0 && (child_bytes == root_drive_path_bytes || self->path[root_drive_path_bytes] == '/')) {
				return 0;
			}
			
			printf("WARNING Path out of bounds; access denied\n");
			return 1;
			
		} case FS_DRIVE_SYSTEM: {
			if (0) self->write_access = 0; // has write permissions?
			if (0) self->read_access = 0; // has read permissions?
			
			strncpy(temp_path, self->raw_path, sizeof(temp_path));
			sprintf(self->path, "/%s", temp_path);
			return 0;
			
		} default: {
			printf("WARNING Unknown drive '%c'; access denied\n", self->drive);
			return 1;
		}
	}
	
	printf("WARNING Unknown error in filtering paths; access denied\n");
	return 1;
}
