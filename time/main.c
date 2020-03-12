
#include <time.h>
#include <stdint.h>
#include <string.h>

uint64_t** kos_bda_pointer = (uint64_t**) 0;

void handle(uint64_t** result_pointer_pointer, char* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	time_t _time = time(NULL);
	
	if (strcmp(data, "unix") == 0) {
		kos_bda[0] = _time;
		
	} else if (strcmp(data, "now") == 0) {
		struct timespec spec;
		clock_gettime(CLOCK_REALTIME, &spec);
		
		kos_bda[0] = (spec.tv_sec * 1000000 + spec.tv_nsec / 1000) & 0xFFFFFFFF;
		if (spec.tv_nsec % 1000 >= 500) kos_bda[0]++; // round up
		
	} else if (strcmp(data, "current") == 0) {
		struct tm* tm_struct = localtime(&_time);
		
		kos_bda[0] = (uint64_t) tm_struct->tm_hour;
		kos_bda[1] = (uint64_t) tm_struct->tm_min;
		kos_bda[2] = (uint64_t) tm_struct->tm_sec;
		
		kos_bda[3] = (uint64_t) tm_struct->tm_wday;
		kos_bda[4] = (uint64_t) tm_struct->tm_mday;
		kos_bda[5] = (uint64_t) tm_struct->tm_yday;
		
		kos_bda[6] = (uint64_t) tm_struct->tm_mon;
		kos_bda[7] = (uint64_t) tm_struct->tm_year;
	}
}
