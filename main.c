#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "algo.h"
#include "args.h"
#include "exp_rand.h"
#include "process.h"

int main(int argc, char* argv[]) {
	args_t* args = parse_args(argc, argv);
	printf("<<< PROJECT PART I -- process set (n=%d) ", args->n);
	printf("with %d CPU-bound process%s >>>\n", args->n_cpu,
	       args->n_cpu == 1 ? "" : "es");

	process_t* processes = generate_processes(args->n, args->n_cpu, args->seed,
	                                          args->lambda, args->exp_max);
	print_processes(processes, args->n, 0);
	printf("\n");

	printf("<<< PROJECT PART II -- t_cs=%ums; alpha=%.2f; t_slice=%lums >>>\n",
	       args->Tcs, args->alpha, args->Tslice);

	process_t* p_copy = dup_process_array(processes, args->n);
	algo_stat_t stats_fcfs = algo_fcfs(args, p_copy);
	printf("\n");

	copy_process_array(p_copy, processes, args->n);
	algo_stat_t stats_sjf = algo_sjf(args, p_copy);
	printf("\n");

	copy_process_array(p_copy, processes, args->n);
	algo_stat_t stats_srt = algo_srt(args, p_copy);
	printf("\n");

	copy_process_array(p_copy, processes, args->n);
	algo_stat_t stats_rr = algo_rr(args, p_copy);

	free_process_array(processes, args->n);
	free_process_array(p_copy, args->n);
	free(p_copy);
	free(processes);
	free(args);
	args = NULL;

	FILE* f = fopen("simout.txt", "w");
	if (f == NULL) {
		perror("ERROR: fopen");
		exit(EXIT_FAILURE);
	}

	fprintf(f, "Algorithm FCFS\n");
	print_algo_stat(f, &stats_fcfs);
	fprintf(f, "\n");

	fprintf(f, "Algorithm SJF\n");
	print_algo_stat(f, &stats_sjf);
	fprintf(f, "\n");

	fprintf(f, "Algorithm SRT\n");
	print_algo_stat(f, &stats_srt);
	fprintf(f, "\n");

	fprintf(f, "Algorithm RR\n");
	print_algo_stat(f, &stats_rr);

	if (fclose(f) != 0) {
		perror("ERROR: fclose");
		exit(EXIT_FAILURE);
	}

	return 0;
}
