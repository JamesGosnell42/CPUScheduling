#include "exp_rand.h"
#include <math.h>
#include <stdlib.h>

void seed_exp(unsigned int seed) {
	srand48(seed); // Correctly seed the random number generator
}

double next_exp(double lambda) {
	double u = drand48();    // Generate U, a uniform random number in [0, 1)
	return -log(u) / lambda; // Return an exponentially distributed random number
}

double ceil_exp(double lambda, double exp_max) {
	double r = ceil(next_exp(lambda));
	while (r > exp_max) { r = ceil(next_exp(lambda)); }
	return r;
}

double floor_exp(double lambda, double exp_max) {
	double r = floor(next_exp(lambda));
	while (r > exp_max) { r = floor(next_exp(lambda)); }
	return r;
}
