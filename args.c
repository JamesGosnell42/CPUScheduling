#include "args.h"
#include <stdio.h>
#include <stdlib.h>

args_t* parse_args(int argc, char* argv[]) {
	args_t* args = calloc(1, sizeof(args_t));

	if (argc != 9) { // All input is required, I didn't see any labeled "optional"
		fprintf(stderr, "ERROR: Number of arguments must be 8\n");
		exit(1);
	}

	if (atoi(argv[1]) > 26 || atoi(argv[1]) < 1) {
		fprintf(stderr,
		        "ERROR: Number of Processes must be at most 26 and at least 1\n");
		exit(1);
	} else {
		args->n = atoi(argv[1]);
	}

	if (atoi(argv[2]) < 0 || atoi(argv[2]) > args->n) {
		fprintf(stderr,
		        "ERROR: Number of CPU-bound Processes must be positive and less "
		        "then or equal to number of processes\n");
		exit(1);
	} else {
		args->n_cpu = atoi(argv[2]);
	}

	args->seed = atol(argv[3]);    // no restrictions
	args->lambda = atof(argv[4]);  // no restrictions
	args->exp_max = atol(argv[5]); // no restricitons

	if (atoi(argv[6]) < 0 || atoi(argv[6]) % 2 != 0) {
		fprintf(stderr,
		        "ERROR: Context Switch Time must be a positive even number\n");
		exit(1);
	} else {
		args->Tcs = atoi(argv[6]);
	}

	if (atof(argv[7]) < 0) {
		fprintf(stderr, "ERROR: Alpha must be positive\n");
		exit(1);
	} else {
		args->alpha = atof(argv[7]);
	}
	if (atoi(argv[8]) < 0) {
		fprintf(stderr, "ERROR: Time Slice must be positive\n");
		exit(1);
	} else {
		args->Tslice = atol(argv[8]);
	}
	return args;
}
