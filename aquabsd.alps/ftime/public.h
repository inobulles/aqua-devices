#if !defined(__AQUABSD_ALPS_FTIME)
#define __AQUABSD_ALPS_FTIME

#include <stdint.h>
#include <stddef.h>

typedef struct {
	double frametime; // frametime itself
	double when; // when was the frametime recorded? this is necessary to flush out outdated frametimes
} aquabsd_alps_ftime_record_t;

typedef struct {
	double target; // what frametimes are we targeting? (1/60 for 60 Hz, 1/144 for 144 Hz, &c)

	size_t record_count; // how many frametimes have we recorded so far?
	aquabsd_alps_ftime_record_t* records; // sorted (largest to smallest) list of recorded frametimes

	double expected_draw_time; // how long do we expect the next draw to take?
	double draw_start; // when did the last draw start?
	double swap_start; // when did the last swap start? (same thing as when the draw ended)

	double draw_time; // how long did the draw take?
	double wait_time; // how long did we decide to wait last time?
	double total_time; // how long was the total frame time (draw + swap + wait times)?
} aquabsd_alps_ftime_t;

// how many frames do we take into account?

#if !defined(AQUABSD_ALPS_FTIME_RECORDED_FTIMES)
	#define AQUABSD_ALPS_FTIME_RECORDED_FTIMES 500
#endif

aquabsd_alps_ftime_t* (*aquabsd_alps_ftime_create) (double target);
double (*aquabsd_alps_ftime_draw) (aquabsd_alps_ftime_t* ftime);
void (*aquabsd_alps_ftime_swap) (aquabsd_alps_ftime_t* ftime);
void (*aquabsd_alps_ftime_done) (aquabsd_alps_ftime_t* ftime);
void (*aquabsd_alps_ftime_delete) (aquabsd_alps_ftime_t* ftime);

#endif
