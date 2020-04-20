#define IAR_MAGIC 0x1A4C1A4C1A4C1A4C

typedef struct {
	uint64_t magic;
	uint64_t version;
	uint64_t root_node_offset;
	uint64_t page_bytes;
} iar_header_t;

typedef struct {
	uint64_t node_count;
	uint64_t node_offsets_offset;
	
	uint64_t name_bytes;
	uint64_t name_offset;
	
	uint64_t data_bytes;
	uint64_t data_offset;
} iar_node_t;

typedef struct {
	FILE* fp;
	int fd;
	
	iar_header_t header;
	iar_node_t root_node;
} iar_file_t;

static uint64_t iar_find_node(iar_file_t* self, iar_node_t* node, const char* name, iar_node_t* parent) { // return file index of found file or -1 if nothing found, 
	uint64_t node_offsets_bytes = parent->node_count * sizeof(uint64_t);
	uint64_t* node_offsets = (uint64_t*) malloc(node_offsets_bytes);
	pread(self->fd, node_offsets, node_offsets_bytes, parent->node_offsets_offset);
	
	uint64_t found = -1;
	for (int i = 0; i < parent->node_count; i++) {
		iar_node_t child_node;
		pread(self->fd, &child_node, sizeof(child_node), node_offsets[i]);
		
		char* node_name = (char*) malloc(child_node.name_bytes);
		pread(self->fd, node_name, child_node.name_bytes, child_node.name_offset);
		
		uint8_t condition = strncmp(name, node_name, sizeof(node_name)) == 0;
		free(node_name);
		
		if (condition) {
			memcpy(node, &child_node, sizeof(child_node));
			found = i;
			break;
		}
	}
	
	free(node_offsets);
	return found;
}

int iar_read_node_contents(iar_file_t* self, iar_node_t* node, char* buffer) {
	if (node->data_offset) {
		pread(self->fd, buffer, node->data_bytes, node->data_offset);
		return 0;
		
	} else {
		printf("WARNING Provided node is not a file and thus contains no data\n");
		return 1;
	}
}

static int iar_map_node_contents(iar_file_t* self, iar_node_t* node, void* address) {
	if (node->data_offset) {
		if (mmap(address, node->data_bytes, PROT_READ, MAP_PRIVATE | MAP_FIXED, self->fd, node->data_offset) == MAP_FAILED) {
			printf("WARNING Couldn't map file to memory (%d)\n", errno);
			return 1;
		}
		
		return 0;
		
	} else {
		printf("WARNING Provided node is not a file and thus contains no data\n");
		return 1;
	}
}

static void iar_free(iar_file_t* self) {
	fclose(self->fp);
}
