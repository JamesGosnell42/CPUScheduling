#include "algo.h"
#include "math.h"

double round_stat(double stat) {
	if (isnan(stat) || isinf(stat))
		return 0.0;
	else
		return ceil(stat * 1000.0) / 1000.0;
}

void print_algo_stat(FILE* stream, algo_stat_t* stat) {
	fprintf(stream, "-- CPU utilization: %2.3f%%\n", round_stat(stat->cpu_util));
	fprintf(stream, "-- average CPU burst time: %.3f ms (%.3f ms/%.3f ms)\n",
	        round_stat(stat->t_burst.avg), round_stat(stat->t_burst.cpu_avg),
	        round_stat(stat->t_burst.io_avg));
	fprintf(stream, "-- average wait time: %.3f ms (%.3f ms/%.3f ms)\n",
	        round_stat(stat->t_wait.avg), round_stat(stat->t_wait.cpu_avg),
	        round_stat(stat->t_wait.io_avg));
	fprintf(stream, "-- average turnaround time: %.3f ms (%.3f ms/%.3f ms)\n",
	        round_stat(stat->t_turn.avg), round_stat(stat->t_turn.cpu_avg),
	        round_stat(stat->t_turn.io_avg));
	fprintf(stream, "-- number of context switches: %d (%d/%d)\n",
	        stat->cs_cpu + stat->cs_io, stat->cs_cpu, stat->cs_io);
	fprintf(stream, "-- number of preemptions: %d (%d/%d)\n",
	        stat->pre_cpu + stat->pre_io, stat->pre_cpu, stat->pre_io);
}

int exp_avg_tau(float alpha, int b_n, int tau_n) {
	return ceil(alpha * b_n + (1.0 - alpha) * tau_n);
}

void stat_avg_add(sim_stat_t* sum, sim_stat_t* ct, double val, int cpu_bound) {
	sum->avg += val;
	if (ct) ct->avg += 1;
	if (cpu_bound) {
		sum->cpu_avg += val;
		if (ct) ct->cpu_avg += 1;
	} else {
		sum->io_avg += val;
		if (ct) ct->io_avg += 1;
	}
}

void stat_cs_inc(algo_stat_t* stat, int cpu_bound) {
	if (cpu_bound) {
		++stat->cs_cpu;
	} else {
		++stat->cs_io;
	}
}

void stat_pre_inc(algo_stat_t* stat, int cpu_bound) {
	if (cpu_bound) {
		++stat->pre_cpu;
	} else {
		++stat->pre_io;
	}
}

void stat_calc_final(algo_stat_t* sum, const algo_stat_t* ct,
                     unsigned long t_total) {
	sum->cpu_util = sum->t_burst.avg / t_total * 100.0;
	sum->t_burst.avg /= ct->t_burst.avg;
	sum->t_burst.cpu_avg /= ct->t_burst.cpu_avg;
	sum->t_burst.io_avg /= ct->t_burst.io_avg;
	sum->t_wait.avg /= ct->t_wait.avg;
	sum->t_wait.cpu_avg /= ct->t_wait.cpu_avg;
	sum->t_wait.io_avg /= ct->t_wait.io_avg;
	sum->t_turn.avg /= ct->t_turn.avg;
	sum->t_turn.cpu_avg /= ct->t_turn.cpu_avg;
	sum->t_turn.io_avg /= ct->t_turn.io_avg;
}
