#ifndef OPSYS_SIM_PROCESS_H_
#define OPSYS_SIM_PROCESS_H_

#include <sys/types.h>

#define MAX_BURSTS 64

typedef struct process {
	char id;
	int cpu_bound;
	int arrival_time;
	int cpu_burst_ct;
	int* cpu_bursts;
	int* io_bursts;
} process_t;

process_t* generate_processes(int n, int n_cpu, int seed, double lambda,
                              int exp_max);

/**
 * Duplicate process array p of length n.
 * @return The newly copied array.
 */
process_t* dup_process_array(process_t* p, int n);

/**
 * Copy process array src into process array dst.
 */
process_t* copy_process_array(process_t* dst, const process_t* src, size_t n);

/**
 *  free calloced elements in a array of processes
 */
void free_process_array(process_t* src, size_t n);
/**
 *  free calloced elements in a processes
 */
void free_process(process_t p);

/**
 * Copy process source into process dest. Arrays are realloc'd as needed.
 */
void copy_process(process_t* dest, const process_t* source);

/**
 * Print process array p.
 * @param p Process array.
 * @param n Process array length.
 * @param print_bursts Boolean option of whether to print each burst.
 */
void print_processes(process_t* p, int n, int print_bursts);


#endif // OPSYS_SIM_PROCESS_H_
