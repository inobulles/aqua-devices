
#include <math.h>
#include <stdint.h>

#define ZXPN_PRECISION 1048576 // 2 ^ 20

// commands

#define CMD_SIN   0x7369 // 'si'
#define CMD_TAN   0x7461 // 'ta'

#define CMD_ASIN  0x6173 // 'as'
#define CMD_ATAN  0x6174 // 'at'
#define CMD_ATAN2 0x6132 // 'a2'

#define CMD_ROOT  0x7274 // 'rt'
#define CMD_POW   0x7077 // 'pw'
#define CMD_LOG   0x6C67 // 'lg'

// helper functions

static inline double root(double power_reciprocal /* the name 'root' is already used by this function */, double x) {
	if (power_reciprocal == 2.0) return sqrt(x);
	return pow(x, 1.0 / power_reciprocal);
}

static inline double a_log(double base, double x) {
	if (base == M_E) return log(x);
	if (base == 10.0) return log10(x);
	return log(x) / log(base);
}

// 🍖🍖🍖

uint64_t send(uint16_t command, void* __data) {
	uint64_t* data = (uint64_t*) __data;

	if (!data) {
		// just a precaution
		return -1;
	}

	// convert each argument from ZXPN (ZED fiXed Point Number) to double-precision IEEE 754 floating point numbers

	uint8_t argument_count = data[0];
	double arguments[2] = { 0.0 }; // none of our commands need more that 2 arguments for now

	for (uint8_t i = 0; i < argument_count; i++) {
		arguments[i] = (double) (int64_t) data[1 + i] / ZXPN_PRECISION;
	}

	// actually do the maffs

	switch (command) {
		case CMD_SIN  : return sin  (arguments[0]);
		case CMD_TAN  : return tan  (arguments[0]);

		case CMD_ASIN : return asin (arguments[0]);
		case CMD_ATAN : return atan (arguments[0]);
		case CMD_ATAN2: return atan2(arguments[0], arguments[1]);

		case CMD_ROOT : return root (arguments[0], arguments[1]);
		case CMD_POW  : return pow  (arguments[0], arguments[1]);
		case CMD_LOG  : return a_log(arguments[0], arguments[1]);
	}

	return -1;
}