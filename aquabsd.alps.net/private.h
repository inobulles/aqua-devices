#define dynamic

#include <aquabsd.alps.net/public.h>

// logging stuff

#define LOG_SIGNATURE "[aquabsd.alps.net]"

#define LOG_REGULAR "\033[0m"
#define LOG_RED     "\033[0;31m"
#define LOG_GREEN   "\033[0;32m"
#define LOG_YELLOW  "\033[0;33m"

#define WARN(...) \
	fprintf(stderr, LOG_SIGNATURE " %s " LOG_YELLOW, __func__); \
	fprintf(stderr, "WARNING "__VA_ARGS__); \
	fprintf(stderr, LOG_REGULAR);

// response stuff

#define response_t aquabsd_alps_net_response_t
