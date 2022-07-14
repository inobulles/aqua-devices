#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))

static inline double __get_time(void) {
	struct timespec timer;
	clock_gettime(CLOCK_MONOTONIC, &timer);

	return timer.tv_sec + 1.e-9 * timer.tv_nsec;
}

ftime_t* create(double target) {
	ftime_t* ftime = calloc(1, sizeof *ftime);

	ftime->target = target;
	ftime->times = malloc(RECORDED_FTIMES * sizeof *ftime->times);

	return ftime;
}

void draw(ftime_t* ftime) {
	// compute 1% lows (in terms of FPS, so 99th percentile in frametimes)
	// that is, what frametime has 99% of other frametimes below it?
	// if we don't yet have any data for this, don't wait at all before drawing

	ftime->expected_draw_time = 0;
	size_t len = RECORDED_FTIMES / 100;

	for (size_t i = 0; i < MIN(ftime->times_count, len); i++) {
		ftime->expected_draw_time += ftime->times[i];
	}

	// add epsilon so division is defined when 'ftime->times_count == 0'

	ftime->expected_draw_time /= ftime->times_count + 0.000001;

	// wait for 'ftime->target' subtracted by the time we expect the draw to take
	// if that time ends up being less than zero, don't wait at all in an attempt to recover performance

	double wait_time = ftime->target - ftime->expected_draw_time;

	if (wait_time < 0) {
		wait_time = 0;
		LOG_WARN("Expecting draw time (%g) to take longer than target (%g); this means things are running slower than they should!")
	}

	int us = wait_time * 1000000;
	usleep(us);

	// we can now start drawing

	ftime->draw_start = __get_time();
}

// swapping happens at the end of the draw

void swap(ftime_t* ftime) {
	// insert time taken to draw to the list of recorded frametimes
	// this may look like O(n^2), but it's actually O(n)

	double draw_end = __get_time();
	double draw_time = draw_end - ftime->draw_start;

	for (size_t i = 0; i < ftime->times_count; i++) {
		double time = ftime->times[i];

		if (draw_time > time) {
			continue;
		}

		// move everything >= i to the right
		// if at the end of the list, forget about it, it's anyway too small to be significant to us

		for (size_t j = ftime->times_count - 1; j >= i; j--) {
			ftime->times[j + 1] = ftime->times[j];
		}

		ftime->times[i] = draw_time;
		goto added_time;
	}

	// draw time is smaller than anything in the list, so add it to the end
	// if list is already full, forget about it, it's anyway too small to be significant to us

	if (ftime->times_count < RECORDED_FTIMES) {
		ftime->times[ftime->times_count++] = draw_time;
	}

added_time:

	ftime->swap_start = draw_end;
}

void done(ftime_t* ftime) {
	// double swap_end = __get_time();
}

void delete(ftime_t* ftime) {
	free(ftime->times);
	free(ftime);
}
