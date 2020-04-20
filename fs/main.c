
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <dirent.h>
#include <string.h>

uint64_t** kos_bda_pointer = (uint64_t**) 0;

#define FS_TYPE_INEXISTENT 0x00
#define FS_TYPE_UNKNOWN    0x01
#define FS_TYPE_DIRECTORY  0x02
#define FS_TYPE_FILE       0x03

#include "utility.h"
#include "drives.h"

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	kos_bda[0] = 0; // be optimistic about the success of the program
	
	if (data[0] == 0x72) { // read
		path_t path;
		if (setup_path(&path, (const char*) data[1])) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		if (!path.read_access) {
			printf("WARNING Read access denied\n");
			kos_bda[0] = 1; // failure
			return;
		}
		
		if (create_path(&path)) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		char** data_pointer = (char**) data[2];
		uint64_t* bytes_pointer = (uint64_t*) data[3];
		
		if (path.drive != FS_DRIVE_ASSETS) {
			//~ FILE* file = fopen(path.path, "rb");
			//~ if (!file) {
				//~ printf("WARNING File not found\n");
				//~ kos_bda[0] = 1; // failure
				//~ return;
			//~ }
			
			//~ fseek(file, 0L, SEEK_END);
			//~ *bytes_pointer = ftell(file);
			//~ rewind(file);
			
			//~ *data_pointer = (char*) malloc(*bytes_pointer);
			//~ fread(*data_pointer, 1, *bytes_pointer, file);
			
			//~ fclose(file);
			
			int fd = open(path.path, O_RDONLY);
			if (fd < 0) {
				printf("WARNING File not found\n");
				kos_bda[0] = 1; // failure
				return;
			}
			
			struct stat file_info;
			if (fstat(fd, &file_info) == -1) {
				close(fd);
				printf("WARNING Couldn't stat file\n");
				kos_bda[0] = 1; // failure
				return;
			}
			
			long page_size = sysconf(_SC_PAGESIZE);
			*bytes_pointer = file_info.st_size;
			
			*data_pointer = mmap((void*) 0, *bytes_pointer + page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) + page_size;
			if (*data_pointer == MAP_FAILED) {
				close(fd);
				printf("WARNING Couldn't allocate memory\n");
				kos_bda[0] = 1; // failure
				return;
			}
			
			*((uint64_t*) (*data_pointer - sizeof(uint64_t))) = *bytes_pointer;
			if (mmap(*data_pointer, *bytes_pointer, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) == MAP_FAILED) {
				munmap(*data_pointer, *bytes_pointer + page_size);
				close(fd);
				
				printf("WARNING Couldn't map file to memory\n");
				kos_bda[0] = 1; // failure
				return;
			}
			
			close(fd);
			return;
		}
		
		iar_node_t parent_node;
		memcpy(&parent_node, &path.assets_node, sizeof(parent_node));
		
		for (int i = 0; i < path.list_index; i++) {
			iar_node_t temp_node;
			if (iar_find_node(boot_package_pointer, &temp_node, path.list[i], &parent_node) == -1) {
				printf("WARNING File not found\n");
				kos_bda[0] = 1; // failure
				
				free_path(&path);
				return;
			}
			
			if (i >= path.list_index - 1) { // at the end of the list (presumably, this is the file node)
				if (!temp_node.data_bytes) {
					printf("WARNING File is empty\n");
					kos_bda[0] = 1; // failure
					
					free_path(&path);
					return;
				}
				
				*bytes_pointer = temp_node.data_bytes;
				*data_pointer = (char*) malloc(*bytes_pointer);
				
				if (iar_read_node_contents(boot_package_pointer, &temp_node, *data_pointer)) {
					printf("WARNING Node is not a file\n");
					kos_bda[0] = 1;
					
					free(*data_pointer);
					free_path(&path);
					return;
				}
				
				break;
			}
		}
		
		free_path(&path);
		return;
		
	} else if (data[0] == 0x77) { // write
		path_t path;
		if (setup_path(&path, (const char*) data[1])) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		if (!path.write_access) {
			printf("WARNING Write access denied\n");
			kos_bda[0] = 1; // failure
			return;
		}
		
		if (create_path(&path)) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		char* write_data = (char*) data[2];
		uint64_t bytes = data[3];
		
		FILE* file = fopen(path.path, "wb");
		if (!file) {
			printf("WARNING File not found\n");
			kos_bda[0] = 1; // failure
			return;
		}
		
		fwrite(write_data, 1, bytes, file);
		fclose(file);
		
	} else if (data[0] == 0x6D) { // create_directory
		path_t path;
		if (setup_path(&path, (const char*) data[1])) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		if (!path.write_access) {
			printf("WARNING Write access denied\n");
			kos_bda[0] = 1; // failure
			return;
		}
		
		if (create_path(&path)) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		if (mkdir(path.path, 0700) < 0) {
			printf("WARNING Directory probably already exists\n");
			kos_bda[0] = 1; // failure (directory probably already exists)
		}
		
	} /*else if (data[0] == 0x76) { // move / copy
		uint64_t copy = data[1];
		
		char* destination_path;
		if (filter_path(&destination_path, (const char*) data[2])) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		char* source_path;
		if (filter_path(&source_path, (const char*) data[2])) {
			kos_bda[0] = 1; // failure
			free(destination_path);
			return;
		}
		
		if (copy) { // copy
			kos_bda[0] = copy_recursive(destination_path, source_path);
			
		} else if (rename(source_path, destination_path) < 0) { // move
			kos_bda[0] = 1; // failure
		}
		
		free(source_path);
		free(destination_path);
		
	} else if (data[0] == 0x64) { // remove
		char* path;
		if (filter_path(&path, (const char*) data[1])) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		kos_bda[0] = remove_recursive(path);
		free(path);
		
	} else if (data[0] == 0x6C) { // list
		char* path;
		if (filter_path(&path, (const char*) data[1])) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		char*** list_pointer = (char***) data[2];
		uint64_t** list_bytes_pointer = (uint64_t**) data[3];
		uint64_t* list_size_pointer = (uint64_t*) data[4];
		
		*list_size_pointer = 0;
		struct dirent* entry;
		
		DIR* dp = opendir(path);
		if (!dp) {
			kos_bda[0] = 1; // failure
			free(path);
			return;
		}
		
		#define VALID(name) (strcmp((name), "..") && strcmp((name), "."))
		while ((entry = readdir(dp)) != NULL) {
			*list_size_pointer += VALID(entry->d_name);
		}
		
		*list_bytes_pointer = (uint64_t*) malloc(*list_size_pointer * sizeof(uint64_t));
		*list_pointer = (char**) malloc(*list_size_pointer * sizeof(char*));
		
		rewinddir(dp);
		uint64_t index = 0;
		
		while ((entry = readdir(dp)) != NULL) {
			const char* name = entry->d_name;
			
			if (VALID(name)) {
				(*list_bytes_pointer)[index] = strlen(name) + 1;
				(*list_pointer)[index] = (char*) malloc((*list_bytes_pointer)[index]);
				
				memcpy((*list_pointer)[index], name, (*list_bytes_pointer)[index]);
				index++;
			}
		}
		
		closedir(dp);
		free(path);
		
	} else if (data[0] == 0x74) { // type
		char* path;
		if (filter_path(&path, (const char*) data[1])) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		DIR* dp = opendir(path);
		if (!dp) {
			kos_bda[0] = FS_TYPE_INEXISTENT; // failure
			free(path);
			return;
		}
		
		struct dirent* entry;
		if ((entry = readdir(dp)) != NULL) {
			kos_bda[0] = FS_TYPE_UNKNOWN;
			
			if (entry->d_type == DT_UNKNOWN) {
				struct stat info;
				stat(entry->d_name, &info);
				
				if      (S_ISREG(info.st_mode)) kos_bda[0] = FS_TYPE_FILE;
				else if (S_ISDIR(info.st_mode)) kos_bda[0] = FS_TYPE_DIRECTORY;
				
			}
			
			else if (entry->d_type == DT_REG) kos_bda[0] = FS_TYPE_FILE;
			else if (entry->d_type == DT_DIR) kos_bda[0] = FS_TYPE_DIRECTORY;
		}
		
		free(path);
	}*/
}

