
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <string.h>

void load(void) {
	
}

void flip(void) {
	
}

void quit(void) {
	
}

uint64_t** kos_bda_pointer = (uint64_t**) 0;

#define FS_TYPE_INEXISTENT 0x00
#define FS_TYPE_UNKNOWN    0x01
#define FS_TYPE_DIRECTORY  0x02
#define FS_TYPE_FILE       0x03

int copy_file(const char* destination_path, const char* source_path) {
	int input;
	if ((input = open(source_path, O_RDONLY)) == -1) {
		return 1;
	}
	
	int output;
	if ((output = creat(destination_path, 0600)) == -1) {
		close(input);
		return 1;
	}
	
	struct stat info = { 0 };
	fstat(input, &info);
	
	off_t bytes_copied = 0;
	int error = sendfile(output, input, &bytes_copied, info.st_size) != -1;
	
	close(output);
	close(input);
	
	return error;
}

int copy_recursive(const char* destination_name, const char* source_name) {
	mkdir(destination_name, 0700);
	int error = 0;
	
	DIR* dp = opendir(source_name);
	if (!dp) {
		goto _exit;
	}
	
	char source_path[4096];
	char destination_path[4096];
	
	struct dirent* entry;
	while ((entry = readdir(dp)) != NULL) {
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
			snprintf(source_path, sizeof(source_path), "%s/%s", source_name, entry->d_name);
			snprintf(destination_path, sizeof(destination_path), "%s/%s", destination_name, entry->d_name);
			
			if (entry->d_type == DT_DIR) error += copy_recursive(destination_path, source_path);
			else error += copy_file(destination_path, source_path);
		}
	}
	
	closedir(dp);
	
_exit:
	error += copy_file(destination_name, source_name);
	return error;
}

int remove_recursive(const char* name) {
	int error = 0;
	
	DIR* dp = opendir(name);
	if (!dp) {
		goto _exit;
	}
	
	char path[4096];
	
	struct dirent* entry;
	while ((entry = readdir(dp)) != NULL) {
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
			snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
			
			if (entry->d_type == DT_DIR) error += remove_recursive(path);
			else error += remove(path) == -1;
		}
	}
	
	closedir(dp);
	
_exit:
	error += remove(name) == -1;
	return error;
}

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	kos_bda[0] = 0; // be optimistic about the success of the program
	
	if (data[0] == 0x72) { // read
		const char* path = (const char*) data[1];
		char** data_pointer = (char**) data[2];
		uint64_t* bytes_pointer = (uint64_t*) data[3];
		
		FILE* file = fopen(path, "rb");
		if (!file) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		fseek(file, 0L, SEEK_END);
		*bytes_pointer = ftell(file);
		rewind(file);
		
		*data_pointer = (char*) malloc(*bytes_pointer);
		fread(*data_pointer, 1, *bytes_pointer, file);
		fclose(file);
		
	} else if (data[0] == 0x77) { // write
		const char* path = (const char*) data[1];
		char* write_data = (char*) data[2];
		uint64_t bytes = data[3];
		
		FILE* file = fopen(path, "wb");
		if (!file) {
			kos_bda[0] = 1; // failure
			return;
		}
		
		fwrite(write_data, 1, bytes, file);
		fclose(file);
				
	} else if (data[0] == 0x6D) { // create_directory
		const char* path = (const char*) data[1];
		
		if (mkdir(path, 0700) < 0) {
			kos_bda[0] = 1; // failure (directory probably already exists)
		}
		
	} else if (data[0] == 0x76) { // move / copy
		uint64_t copy = data[1];
		const char* destination_path = (const char*) data[2];
		const char* source_path = (const char*) data[3];
		
		if (copy) { // copy
			kos_bda[0] = copy_recursive(destination_path, source_path);
			
		} else if (rename(source_path, destination_path) < 0) { // move
			kos_bda[0] = 1; // failure
		}
		
	} else if (data[0] == 0x64) { // remove
		const char* path = (const char*) data[1];
		kos_bda[0] = remove_recursive(path);
		
	} else if (data[0] == 0x6C) { // list
		const char* path = (const char*) data[1];
		char*** list_pointer = (char***) data[2];
		uint64_t** list_bytes_pointer = (uint64_t**) data[3];
		uint64_t* list_size_pointer = (uint64_t*) data[4];
		
		*list_size_pointer = 0;
		struct dirent* entry;
		
		DIR* dp = opendir(path);
		if (!dp) {
			kos_bda[0] = 1; // failure
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
		
	} else if (data[0] == 0x74) { // type
		const char* path = (const char*) data[1];
		
		DIR* dp = opendir(path);
		if (!dp) {
			kos_bda[0] = FS_TYPE_INEXISTENT; // failure
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
	}
}

