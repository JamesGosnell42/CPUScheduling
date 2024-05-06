#ifndef OPSYS_SIM_EXP_RAND_H_
#define OPSYS_SIM_EXP_RAND_H_

void seed_exp(unsigned int seed);
double next_exp(double lambda);

// Generate ceil(next_exp(lambda)), skipping values > exp_max.
double ceil_exp(double lambda, double exp_max);

// Generate floor(next_exp(lambda)), skipping values > exp_max.
double floor_exp(double lambda, double exp_max);

#endif // OPSYS_SIM_EXP_RAND_H_
