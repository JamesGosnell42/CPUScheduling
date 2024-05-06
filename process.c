#include "process.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "exp_rand.h"

process_t* generate_processes(int n, int n_cpu, int seed, double lambda,
                              int exp_max) {
	seed_exp(seed);
	process_t* p = calloc(n, sizeof(process_t));
	for (int i = 0; i < n; ++i) {
		p[i].id = 'A' + i;
		p[i].arrival_time = floor_exp(lambda, exp_max);
		p[i].cpu_burst_ct = ceil(drand48() * MAX_BURSTS);

		p[i].cpu_bursts = calloc(p[i].cpu_burst_ct, sizeof(int));
		p[i].io_bursts = calloc(p[i].cpu_burst_ct - 1, sizeof(int));

		// Get burst times.
		for (int j = 0; j < p[i].cpu_burst_ct - 1; ++j) {
			p[i].cpu_bursts[j] = ceil_exp(lambda, exp_max);
			p[i].io_bursts[j] = ceil_exp(lambda, exp_max) * 10;
		}
		p[i].cpu_bursts[p[i].cpu_burst_ct - 1] = ceil_exp(lambda, exp_max);

		// Re-scale CPU-bound processes.
		if (i < (n - n_cpu)) {
			p[i].cpu_bound = 0;
		} else {
			p[i].cpu_bound = 1;
			for (int j = 0; j < p[i].cpu_burst_ct - 1; ++j) {
				p[i].cpu_bursts[j] *= 4;
				p[i].io_bursts[j] /= 8;
			}
			p[i].cpu_bursts[p[i].cpu_burst_ct - 1] *= 4;
		}
	}

	return p;
}

void print_processes(process_t* p, int n, int print_bursts) {
	for (int i = 0; i < n; ++i) {
		// Print header.
		if (p[i].cpu_bound == 1) {
			printf("CPU");
		} else {
			printf("I/O");
		}
		printf("-bound process %c: ", p[i].id);
		printf("arrival time %dms; ", p[i].arrival_time);
		printf("%d CPU burst%s%s\n", p[i].cpu_burst_ct,
		       p[i].cpu_burst_ct == 1 ? "" : "s", print_bursts == 1 ? ":" : "");

		if (print_bursts) {
			for (int j = 0; j < p[i].cpu_burst_ct - 1; ++j) {
				printf("--> CPU burst %dms", p[i].cpu_bursts[j]);
				printf(" --> I/O burst %dms\n", p[i].io_bursts[j]);
			}

			// Print final burst.
			printf("--> CPU burst %dms\n", p[i].cpu_bursts[p[i].cpu_burst_ct - 1]);
		}
	}
}

process_t* dup_process_array(process_t* p, int n) {
	process_t* c = calloc(n, sizeof(process_t));
	copy_process_array(c, p, n);
	return c;
}

process_t* copy_process_array(process_t* dst, const process_t* src, size_t n) {
	for (size_t i = 0; i < n; ++i) copy_process(&dst[i], &src[i]);
	return dst;
}
void free_process_array(process_t* src, size_t n) {
	for (size_t i = 0; i < n; ++i) free_process(src[i]);
}
void free_process(process_t p) {
	free(p.cpu_bursts);
	free(p.io_bursts);
}


/**
 * Copy process source into process dest.
 */
void copy_process(process_t* dest, const process_t* source) {
	dest->id = source->id;
	dest->cpu_bound = source->cpu_bound;
	dest->arrival_time = source->arrival_time;
	dest->cpu_burst_ct = source->cpu_burst_ct;

	if (dest->cpu_burst_ct > 0) {
	dest->cpu_bursts =
	    realloc(dest->cpu_bursts, dest->cpu_burst_ct * sizeof(int));
	} else {
		dest->cpu_bursts = NULL;
	}

	if (dest->cpu_burst_ct > 1) {
		dest->io_bursts =
	    realloc(dest->io_bursts, (dest->cpu_burst_ct - 1) * sizeof(int));
	} else {
		dest->io_bursts = NULL;
	}

	// Copy burst times.
	for (int j = 0; j < dest->cpu_burst_ct - 1; ++j) {
		dest->cpu_bursts[j] = source->cpu_bursts[j];
		dest->io_bursts[j] = source->io_bursts[j];
	}
	dest->cpu_bursts[dest->cpu_burst_ct - 1] =
	    source->cpu_bursts[source->cpu_burst_ct - 1];
}
