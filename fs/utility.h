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
