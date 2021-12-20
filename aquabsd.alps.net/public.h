#if !defined(__AQUABSD_ALPS_NET)
#define __AQUABSD_ALPS_NET

#include <stdio.h>

typedef struct {
	int code;
	FILE* fp;
} aquabsd_alps_net_response_t;

#endif
