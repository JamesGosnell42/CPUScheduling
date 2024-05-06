#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "algo.h"
#include "queue.h"

/*
The SRT algorithm is a preemptive version of the SJF algorithm. In SRT, when a
process arrives, if it has a predicted CPU burst time that is less than the
remaining predicted time of the currently running process, a preemption occurs.
When such a preemption occurs, the currently running process is simply added to
the ready queue.

If different types of events occur at the same time, simulate these events in
the following order: (a) CPU burst completion; (b) process starts using the CPU;
(c) I/O burst completions; and (d) new process arrivals. any “ties” that occur
within one of these categories are to be broken using process ID order
(alphabetically)
*/

/*
OUTPUT
display a summary for each “interesting” event that occurs from time 0 through
time 9999. For time 10000 and beyond, only display process termination events
and the final end-of-simulation even as s your simulation proceeds, t advances
to each “interesting” event that occurs, displaying a specific line of output
that describes each event The “interesting” events are: • Start of simulation •
Process arrival (i.e., initially and at I/O completions) • Process starts using
the CPU • Process finishes using the CPU (i.e., completes a CPU burst) • Process
has its τ value recalculated (i.e., after a CPU burst completion) • Process
preemption • Process starts performing I/O • Process finishes performing I/O •
Process terminates by finishing its last CPU burst • End of simulatio

-- CPU utilization: 83.112%
-- average CPU burst time: 3067.776 ms (4071.000 ms/992.138 ms)
-- average wait time: 542.686 ms (290.800 ms/1063.828 ms)
-- average turnaround time: 3614.911 ms (4366.467 ms/2059.966 ms)
-- number of context switches: 99 (70/29)
-- number of preemptions: 10 (10/0)

*/
// each CPU burst, you will measure CPU burst time (given), turnaround time, and
// wait time number of preemptions number of context switche CPU utilization by
// tracking CPU usage and CPU idle time

enum event_type {
	EV_PROC_CPU_STOP = 0,
	EV_PROC_CPU_PREEMPTION,
	EV_PROC_CPU_START,
	EV_PROC_IO_STOP,
	EV_PROC_CPU_CS,
	EV_PROC_ARRIVAL

};

typedef struct {
	unsigned time;
	char id;
	enum event_type type;
	int burst;
} event_t;

int Q_event_cmp_srt(const void* lhs, const void* rhs) {
	const event_t *lhe = lhs, *rhe = rhs;
	int d_time = lhe->time - rhe->time;
	int d_type = (int) lhe->type - (int) rhe->type;
	int d_id = lhe->id - rhe->id;

	return d_time != 0 ? d_time : (d_type != 0 ? d_type : d_id);
}

typedef struct {
	char id;
	unsigned tau;
	int burst;
	int spent;
	unsigned t_join;
	unsigned p_join;
} ready_t;

int Q_ready_cmp_srt(const void* lhs, const void* rhs) {
	const ready_t *lhg = lhs, *rhg = rhs;
	int d_tau = (lhg->tau - lhg->spent) - (rhg->tau - rhg->spent);
	return d_tau == 0 ? lhg->id - rhg->id : d_tau;
}


void print_ready_queue_srt(queue_t* q) {
	queue_t* q2 = make_queue();
	queue_copy(q2, q);
	printf("[Q");
	if (queue_peek(q2) == NULL) {
		printf(" <empty>");
	} else {
		for (ready_t* g = queue_pop(q2); g; g = queue_pop(q2)) {
			printf(" %c", g->id);
		}
	}
	printf("]");
	free_queue(&q2);
}

void print_event_queue_srt(queue_t* q) {
	queue_t* q2 = make_queue();
	queue_copy(q2, q);
	printf("[Q");
	if (queue_peek(q2) == NULL) {
		printf(" <empty>");
	} else {
		for (event_t* g = queue_pop(q2); g; g = queue_pop(q2)) {
			printf(" %c, %d", g->id, g->type);
		}
	}
	printf("]");
	free_queue(&q2);
}

#ifdef DEBUG_MODE
#define DALWAYS_PRINT 1
#else
#define DALWAYS_PRINT 0
#endif


#define print_event(t, always_print, str, Q) \
	do { \
		if (DALWAYS_PRINT || always_print || t < 10000) { \
			printf("time %ums: " str " ", t); \
			print_ready_queue_srt(Q); \
			printf("\n"); \
		} \
	} while (0)

#define printf_event(t, always_print, fmt, Q, ...) \
	do { \
		if (DALWAYS_PRINT || always_print || t < 10000) { \
			printf("time %ums: " fmt " ", t, __VA_ARGS__); \
			print_ready_queue_srt(Q); \
			printf("\n"); \
		} \
	} while (0)

void add_stat_srt(sim_stat_t* sum, sim_stat_t* ct, double val, int cpu_bound) {
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
void add_stat_preemt(algo_stat_t *srt_stats,int cpu_bound) {
	if(cpu_bound){
		srt_stats->pre_cpu++;
	}else{
		srt_stats->pre_io++;
	}
}

algo_stat_t algo_srt(const args_t* args, process_t* procs) {
	algo_stat_t srt_stats = {0}, srt_counts = {0};

	queue_t *Q_event = make_queue(), *Q_ready = make_queue();
	queue_set_cmp(Q_event, Q_event_cmp_srt);
	queue_set_cmp(Q_ready, Q_ready_cmp_srt);

	ready_t* guesses = calloc(args->n, sizeof(ready_t));

	for (int i = 0; i < args->n; ++i) {
		for (int j = 0; j < procs[i].cpu_burst_ct; ++j){
			add_stat_srt(&srt_stats.t_burst, &srt_counts.t_burst, procs[i].cpu_bursts[j], procs[i].cpu_bound);
		}
		event_t* e = malloc(sizeof(event_t));
		*e = (event_t){.time = procs[i].arrival_time,
		               .id = procs[i].id,
		               .type = EV_PROC_ARRIVAL,
		               .burst = 0};

		queue_push(Q_event, e);

		guesses[i].id = procs[i].id;
		guesses[i].tau = ceil(1 / args->lambda);
		guesses[i].burst = 0;
		guesses[i].spent = 0;
	}

	// Exponential averaging: tau_n+1=alpha(b_n+tau_n) where b are burst times.

	unsigned int t = 0;
	print_event(t, 1, "Simulator started for SRT", Q_ready);

	enum cpu_mode { CM_IDLE = 0, CM_BURST, CM_CS } cpu_mode = CM_IDLE;

	// FIXME: track measurements.
	event_t* currburst = malloc(sizeof(event_t));
	event_t* currstop;
	currburst->id = '#';


	int srt_error = 0;
	while (srt_error == 0 && queue_peek(Q_event)) {
		event_t* e = queue_pop(Q_event);


		t = e->time;

		switch (e->type) {
		case EV_PROC_CPU_STOP: {

			add_stat_srt(&srt_stats.t_turn, &srt_counts.t_turn, t + args->Tcs / 2 - guesses[e->id - 'A'].t_join, procs[e->id - 'A'].cpu_bound);
			char idd = e->id;
			int bursts_left = procs[e->id - 'A'].cpu_burst_ct - 1 - e->burst;
			if (bursts_left == 0) {
				printf_event(t, 1, "Process %c terminated", Q_ready, e->id);
				free(e);

			} else {
				unsigned tau_n = guesses[e->id - 'A'].tau;
				unsigned burst_len = procs[e->id - 'A'].cpu_bursts[e->burst];
				printf_event(t, 0,
				             "Process %c (tau %ums) completed a CPU burst; %d "
				             "burst%s to go",
				             Q_ready, e->id, tau_n, bursts_left,
				             bursts_left == 1 ? "" : "s");



				guesses[e->id - 'A'].tau = exp_avg_tau(args->alpha, (burst_len + guesses[e->id - 'A'].spent), tau_n);

				printf_event(t, 0,
				             "Recalculating tau for process %c: old tau %ums"
				             " ==> new tau %ums",
				             Q_ready, e->id, tau_n, guesses[e->id - 'A'].tau);

				// Requeue IO burst completion.
				e->time = t + (args->Tcs / 2 + procs[e->id - 'A'].io_bursts[e->burst]);
				e->type = EV_PROC_IO_STOP;
				queue_push(Q_event, e);

				printf_event(t, 0,
				             "Process %c switching out of CPU; blocking on I/O"
				             " until time %ums",
				             Q_ready, e->id, e->time);
				currburst->id = '#';
				guesses[e->id - 'A'].spent = 0;
			}
			
			// Simulate context switch.
			
			cpu_mode = CM_CS;
			event_t* e_out = malloc(sizeof(event_t));

			*e_out = (event_t){.time = t + args->Tcs / 2, .type = EV_PROC_CPU_CS};
			e_out->id=idd;
			queue_push(Q_event, e_out);

			break;
		}
		case EV_PROC_CPU_START: {


			if (cpu_mode != CM_BURST) {
				unsigned burst_len = procs[e->id - 'A'].cpu_bursts[e->burst];

				if (guesses[e->id - 'A'].spent != 0) {
					printf_event(t, 0,
					             "Process %c (tau %ums) started using the "
					             "CPU "
					             "for remaining %ums of %ums burst",
					             Q_ready, e->id, guesses[e->id - 'A'].tau, burst_len,
					             burst_len + guesses[e->id - 'A'].spent);
				} else {

					printf_event(t, 0,
					             "Process %c (tau %ums) started "
					             "using the CPU "
					             "for %ums burst",
					             Q_ready, e->id, guesses[e->id - 'A'].tau, burst_len);
				}

				if (queue_peek(Q_ready) != NULL) {
					ready_t* r = queue_peek(Q_ready);
					int left = guesses[e->id - 'A'].tau - guesses[e->id - 'A'].spent;
					int tau = (guesses[r->id - 'A'].tau - guesses[r->id - 'A'].spent);
					if (left > tau) {
						printf_event(t, 0, "Process %c (tau %ums) will preempt %c", Q_ready,
						             r->id, guesses[r->id - 'A'].tau, e->id);
					
					
					cpu_mode = CM_CS;
					e->time = t + (args->Tcs / 2);
					e->type = EV_PROC_CPU_PREEMPTION;
					queue_push(Q_event, e);

					currburst->id = '#';
					break;
					}
				}
				

				// Set CPU_mode.
				cpu_mode = CM_BURST;
				
				// Requeue CPU burst completion.
				*currburst = *e;


				e->time = t + burst_len;
				e->type = EV_PROC_CPU_STOP;
				currstop = e;
				queue_push(Q_event, e);



				
			}else{
				free(e);
			}
			
			break;
		}
		case EV_PROC_IO_STOP: {

			guesses[e->id - 'A'].burst = e->burst + 1;
			guesses[e->id - 'A'].p_join=t;
			guesses[e->id - 'A'].t_join = t;
			if (currburst->id != '#') {
				int left = (guesses[currburst->id - 'A'].tau - (t - currburst->time) -
				            guesses[currburst->id - 'A'].spent);
				int tau = guesses[e->id - 'A'].tau; // TODO: may need add - spent

				if (left > tau) { // if the current burst has less estimated tie

					e->time = t;
				
					queue_push(Q_ready, &guesses[e->id - 'A']);

					printf_event(t, 0,
					             "Process %c (tau %ums) completed I/O; preempting %c",
					             Q_ready, e->id, guesses[e->id - 'A'].tau, currburst->id);

					guesses[currburst->id - 'A'].spent += e->time - currburst->time;
					procs[currburst->id - 'A'].cpu_bursts[currburst->burst] -=
					    e->time - currburst->time;



					// simulate context switch
					cpu_mode = CM_CS;
					

					e->id = currburst->id;
					e->time = t + (args->Tcs / 2);
					e->type = EV_PROC_CPU_PREEMPTION;


					queue_push(Q_event, e);

					size_t place = queue_search(Q_event, currstop);

					if (place != (size_t) -1) { queue_delete(Q_event, place); free(currstop);}
					


					currburst->id = '#';
					break;
				}
			}
			// if the new arrival has less estimated time
		
			queue_push(Q_ready, &guesses[e->id - 'A']);
			printf_event(t, 0,
			             "Process %c (tau %ums) completed I/O; "
			             "added to ready queue",
			             Q_ready, e->id, guesses[e->id - 'A'].tau);


			free(e);
			break;
		}
		case EV_PROC_ARRIVAL: {
			// Add to ready queue.
			guesses[e->id - 'A'].burst = 0;
			guesses[e->id - 'A'].t_join = t;
			guesses[e->id - 'A'].p_join = t;
			if (currburst->id != '#') {
				int left = (guesses[currburst->id - 'A'].tau - (t - currburst->time) -
				            guesses[currburst->id - 'A'].spent);
				int tau = guesses[e->id - 'A'].tau; // TODO: may need add - spent
				if (left > tau) { // if the current burst has less estimated time

					e->time = t;

				   
					queue_push(Q_ready, &guesses[e->id - 'A']);


					procs[currburst->id - 'A'].cpu_bursts[currburst->burst] -=
					    e->time - currburst->time;
					guesses[currburst->id - 'A'].spent = e->time - currburst->time;



					// simulate context switch

					cpu_mode = CM_CS;

					e->id = currburst->id;
					e->time = t + (args->Tcs / 2);
					e->type = EV_PROC_CPU_PREEMPTION;
					queue_push(Q_event, e);


					size_t place = queue_search(Q_event, currstop);
					if (place != (size_t) -1) {
						queue_delete(Q_event, queue_search(Q_event, currstop));
					}


					currburst->id = '#';
					break;
				}
			}
			// if the new arrival has less estimated time
			
			queue_push(Q_ready, &guesses[e->id - 'A']);
			printf_event(t, 0, "Process %c (tau %ums) arrived; added to ready queue",
			             Q_ready, e->id, guesses[e->id - 'A'].tau);

			free(e);

			break;
		}
		case EV_PROC_CPU_PREEMPTION: {

			guesses[e->id - 'A'].p_join=t;
			queue_push(Q_ready, &guesses[e->id - 'A']);
			
			add_stat_preemt(&srt_stats, procs[e->id - 'A'].cpu_bound);
			cpu_mode = CM_CS;
			e->type = EV_PROC_CPU_CS;
			queue_push(Q_event, e);

			break;
		}
		case EV_PROC_CPU_CS: {

			if (procs[e->id - 'A'].cpu_bound) {
					srt_stats.cs_cpu += 1;
			} else {
					srt_stats.cs_io += 1;
			}
			cpu_mode = CM_IDLE;

			free(e);

			break;
		}

		default:
			fprintf(stderr, "ERROR: invalid event type %d.\n", e->type);
			srt_error = 1;
			free(e);
			break;
		}
		if (cpu_mode == CM_IDLE) {

			ready_t* r = queue_pop(Q_ready);

			if (r) {
				event_t* e_start = malloc(sizeof(event_t));
				*e_start = (event_t){.time = t + args->Tcs / 2,
				                     .id = r->id,
				                     .type = EV_PROC_CPU_START,
				                     .burst = r->burst};
				queue_push(Q_event, e_start);
				cpu_mode = CM_CS;
				
				stat_avg_add(&srt_stats.t_wait, &srt_counts.t_wait,t - r->p_join, procs[r->id - 'A'].cpu_bound);
				
			}
		}
	}

	print_event(t, 1, "Simulator ended for SRT", Q_ready);

	free_queue(&Q_ready);
	free_queue(&Q_event);
	free(guesses);
	free(currburst);
	srt_counts.t_wait.avg = srt_counts.t_burst.avg;
	srt_counts.t_wait.cpu_avg = srt_counts.t_burst.cpu_avg;
	srt_counts.t_wait.io_avg = srt_counts.t_burst.io_avg;
stat_calc_final(&srt_stats, &srt_counts, t);

	return srt_stats;
}
