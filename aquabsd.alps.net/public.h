#if !defined(__AQUABSD_ALPS_NET)
#define __AQUABSD_ALPS_NET

#include <stdio.h>
#include <stdint.h>

typedef struct {
	int status;
	FILE* fp;
} aquabsd_alps_net_response_t;

#endif
