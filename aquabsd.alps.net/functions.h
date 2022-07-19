#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// includes required by 'fetch.h'
// TODO probably include them in the fetch header on aquaBSD

#include <time.h>
#include <sys/param.h>

#include <fetch.h>

#define CHUNK_BYTES 4096 // TODO 4k chunks... perhaps should be a client-settable option?

dynamic response_t* get_response(const char* url) {
	response_t* resp = calloc(1, sizeof *resp);
	resp->fp = fetchGetURL(url, "");

	if (!resp->fp) {
		// TODO actually get response code and return NULL here instead
		resp->status = 404;
	}

	return resp;
}

dynamic char* read_response(response_t* resp) {
	if (!resp->fp) {
		return NULL;
	}

	unsigned len = 0;
	char* buf = calloc(1, 1); // empty string

	size_t chunk_bytes;
	uint8_t chunk[CHUNK_BYTES];

	while ((chunk_bytes = fread(chunk, 1, sizeof chunk, resp->fp)) > 0) {
		len += chunk_bytes;
		buf = realloc(buf, len + 1);

		memcpy(buf + len - chunk_bytes, chunk, chunk_bytes);
		buf[len] = '\0'; // null-terminate just incase
	}

	return buf;
}

dynamic int free_response(response_t* resp) {
	if (resp->fp) {
		fclose(resp->fp);
	}

	free(resp);
	return 0;
}