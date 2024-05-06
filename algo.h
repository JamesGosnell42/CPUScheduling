#ifndef OPSYS_SIM_ALGO_H_
#define OPSYS_SIM_ALGO_H_

#include <stdio.h>
#include "args.h"
#include "process.h"

typedef struct {
	double avg;
	double cpu_avg;
	double io_avg;
} sim_stat_t;

typedef struct {
	double cpu_util;
	sim_stat_t t_burst; // CPU burst time
	sim_stat_t t_wait;  // wait time
	sim_stat_t t_turn;  // turnaround time
	int cs_cpu;         // Context switches (CPU-bound)
	int cs_io;          // Context switches (IO-bound)
	int pre_cpu;        // Preemptions (CPU-bound)
	int pre_io;         // Preemptions (IO-bound)
} algo_stat_t;

void print_algo_stat(FILE* stream, algo_stat_t* stat);

algo_stat_t algo_fcfs(const args_t* args, process_t* procs);
algo_stat_t algo_sjf(const args_t* args, process_t* procs);
algo_stat_t algo_srt(const args_t* args, process_t* procs);
algo_stat_t algo_rr(const args_t* args, process_t* procs);

int exp_avg_tau(float alpha, int b_n, int tau_n);

/**
 * Increment a sum and count for an averaged statistic.
 *
 * @param sum The sum to add to.
 * @param ct The count to increment. If ct == NULL, nothing is incremented.
 * @param val Value to add.
 * @param cpu_bound Whether the value is for a CPU-bound or I/O-bound process.
 */
void stat_avg_add(sim_stat_t* sum, sim_stat_t* ct, double val, int cpu_bound);

/**
 * Increment the context switch counters on a statistic object.
 */
void stat_cs_inc(algo_stat_t* stat, int cpu_bound);

/**
 * Increment the preemption switch counters on a statistic object.
 */
void stat_pre_inc(algo_stat_t* stat, int cpu_bound);

/*
 * Calculates final statistics with sums and counts and stores the averages
 * in sum.
 *
 * @param sum Input sum and final stats output location.
 * @param ct Statistical counts for averages.
 * @param t_total Total simulation runtime.
 */
void stat_calc_final(algo_stat_t* sum, const algo_stat_t* ct,
                     unsigned long t_total);

#endif // OPSYS_SIM_ALGO_H_
