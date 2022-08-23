#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

static inline double __get_time(void) {
	struct timespec timer;
	clock_gettime(CLOCK_MONOTONIC, &timer);

	return timer.tv_sec + 1.e-9 * timer.tv_nsec;
}

ftime_t* create(double target) {
	ftime_t* ftime = calloc(1, sizeof *ftime);

	ftime->target = target;
	ftime->records = malloc(RECORDED_FTIMES * sizeof *ftime->records);

	return ftime;
}

double draw(ftime_t* ftime) {
	ftime->draw_start = __get_time();
	return ftime->total_time;
}

// swapping happens at the end of the draw

void swap(ftime_t* ftime) {
	double now = __get_time();
	ftime->draw_time = now - ftime->draw_start;

	if (ftime->expected_draw_time < ftime->draw_time) {
		LOG_VERBOSE("Draw time expectation wasn't conservative enough")
	}

	// insert time taken to draw to the list of recorded frametimes
	// this may look like O(n^2), but it's actually O(n)

	for (ssize_t i = 0; i < ftime->record_count; i++) {
		record_t* record = &ftime->records[i];

		if (ftime->draw_time < record->frametime) {
			continue;
		}

		// move everything >= i to the right
		// if at the end of the list, forget about it, it's anyway too small to be significant to us

		for (ssize_t j = ftime->record_count - 1; j >= i; j--) {
			memcpy(&ftime->records[j + 1], &ftime->records[j], sizeof *record);
		}

		ftime->records[i].frametime = ftime->draw_time;
		ftime->records[i].when = now;

		goto added_time;
	}

	// draw time is smaller than anything in the list, so add it to the end
	// if list is already full, forget about it, it's anyway too small to be significant to us

	if (ftime->record_count < RECORDED_FTIMES) {
		ftime->records[ftime->record_count].frametime = ftime->draw_time;
		ftime->records[ftime->record_count].when = now;

		ftime->record_count++;
	}

added_time:

	// replace records which are too old (older than 1 second) by the next record in the list (smaller)

	for (ssize_t i = 0; i < ftime->record_count - 1; i++) {
		record_t* record = &ftime->records[i];
		double record_age = now - record->when;

		if (record_age < 0.3) {
			continue;
		}

		record_t* next = &ftime->records[i + 1];

		record->frametime = next->frametime;
		record->when = now;
	}

	ftime->swap_start = now;
}

void done(ftime_t* ftime) {
	// how long did the swap take?
	// if swap time + draw time + wait time is not within 40% of 'ftime->target', maybe it isn't correct

	#define DIVERGENCE_TOLERANCE (1 + 0.40)

	double swap_time = __get_time() - ftime->swap_start;
	ftime->total_time = swap_time + ftime->draw_time + ftime->wait_time;

	if (
		ftime->total_time > ftime->target * DIVERGENCE_TOLERANCE ||
		ftime->total_time < ftime->target / DIVERGENCE_TOLERANCE
	) {
		LOG_VERBOSE("Total frame time (draw + swap + wait times = %g) is not within %g%% of the target (%g); this means the target passed is likely incorrect", ftime->total_time, (DIVERGENCE_TOLERANCE - 1) * 100, ftime->target)
	}

	// compute 1% lows (in terms of FPS, so 99th percentile in frametimes)
	// that is, what frametime has 99% of other frametimes below it?
	// if we don't yet have any data for this, don't wait at all before drawing

	ftime->expected_draw_time = 0;
	size_t len = RECORDED_FTIMES / 100;

	for (size_t i = 0; i < MIN(ftime->record_count, len); i++) {
		ftime->expected_draw_time += ftime->records[i].frametime;
	}

	ftime->expected_draw_time /= len;

	// wait for 'ftime->target' subtracted by the time we expect the draw to take
	// if that time ends up being less than zero, don't wait at all in an attempt to recover performance

	ftime->wait_time = ftime->target - ftime->expected_draw_time;

	if (ftime->wait_time < 0) {
		ftime->wait_time = 0;
		LOG_VERBOSE("Expecting draw time (%g) to take longer than target (%g); this means things are running slower than they should!", ftime->expected_draw_time, ftime->target);
	}

	#define CONSERVANCY 1.2
	ftime->wait_time /= CONSERVANCY;

	useconds_t us = ftime->wait_time * 1000000;
	usleep(us);
}

void delete(ftime_t* ftime) {
	free(ftime->records);
	free(ftime);
}
