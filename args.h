#ifndef OPSYS_SIM_ARGS_H
#define OPSYS_SIM_ARGS_H

typedef struct args {
	int n;                     // Number of processes to simulate
	int n_cpu;                 // Number of CPU-bound processes
	long int seed;             // Seed for the random number generator
	double lambda;             // Lambda for exponential distribution
	unsigned long int exp_max; // Upper bound for valid pseudo-random numbers
	unsigned int Tcs;          // Context switch time.
	                  // (Tcs/2 ) is time required to remove the process from CPU;
	                  // the second half of the context switch time is the time
	                  // required to bring the next process in to use the CPU.
	float alpha; // Constant alpha for the SJF and SRT algorithms
	             // note that the initial guess for each processis τ0 = 1/λ .
	             // When calculating τ values, use the “ceiling” function for all
	             // calculations.
	unsigned long int Tslice; // Time Slice value for the RR, in milliseconds.
} args_t;

args_t* parse_args(int argc, char* argv[]);

#endif
