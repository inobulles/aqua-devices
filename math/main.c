
#include <math.h>
#include <stdint.h>

#define PRECISION 1000000.0

double root(double index, double x) {
	if (index == 2.0) return sqrt(x);
	return pow(x, 1.0 / index);
}

double log_base(double base, double x) {
	if (base == 2.718282) return log(x);
	if (base == 10.0) return log10(x);
	return log(x) / log(base);
}

double sigmoid(double x) {
	return 1.0 / (1.0 + exp(-x));
}

uint64_t** kos_bda_pointer = (uint64_t**) 0;

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	
	switch (data[0]) {
		case 0x6E6973         /* sin     */: kos_bda[0] = (int64_t) (sin     ((double) (int64_t) data[1] / PRECISION                                        ) * PRECISION); break;
		case 0x6E6174         /* tan     */: kos_bda[0] = (int64_t) (tan     ((double) (int64_t) data[1] / PRECISION                                        ) * PRECISION); break;
		case 0x6E697361       /* asin    */: kos_bda[0] = (int64_t) (asin    ((double) (int64_t) data[1] / PRECISION                                        ) * PRECISION); break;
		case 0x6E617461       /* atan    */: kos_bda[0] = (int64_t) (atan    ((double) (int64_t) data[1] / PRECISION                                        ) * PRECISION); break;
		case 0x326E617461     /* atan2   */: kos_bda[0] = (int64_t) (atan2   ((double) (int64_t) data[1] / PRECISION, (double) (int64_t) data[2] / PRECISION) * PRECISION); break;
		case 0x686E6973       /* sinh    */: kos_bda[0] = (int64_t) (sinh    ((double) (int64_t) data[1] / PRECISION                                        ) * PRECISION); break;
		case 0x68736973       /* cosh    */: kos_bda[0] = (int64_t) (cosh    ((double) (int64_t) data[1] / PRECISION                                        ) * PRECISION); break;
		case 0x686E6174       /* tanh    */: kos_bda[0] = (int64_t) (tanh    ((double) (int64_t) data[1] / PRECISION                                        ) * PRECISION); break;
		case 0x746F6F72       /* root    */: kos_bda[0] = (int64_t) (root    ((double) (int64_t) data[1] / PRECISION, (double) (int64_t) data[2] / PRECISION) * PRECISION); break;
		case 0x707865         /* exp     */: kos_bda[0] = (int64_t) (exp     ((double) (int64_t) data[1] / PRECISION                                        ) * PRECISION); break;
		case 0x676F6C         /* log     */: kos_bda[0] = (int64_t) (log_base((double) (int64_t) data[1] / PRECISION, (double) (int64_t) data[2] / PRECISION) * PRECISION); break;
		case 0x64696F6D676973 /* sigmoid */: kos_bda[0] = (int64_t) (sigmoid ((double) (int64_t) data[1] / PRECISION                                        ) * PRECISION); break;
	}
}
