
#define LINUX_UNSUPPORTED WARN("Currently unsupported on Linux\n")

dynamic response_t* get_response(const char* url) {
	LINUX_UNSUPPORTED
	return NULL;
}

dynamic char* read_response(response_t* resp) {
	LINUX_UNSUPPORTED
	return NULL;
}

dynamic int free_response(response_t* resp) {
	LINUX_UNSUPPORTED
	return -1;
}
