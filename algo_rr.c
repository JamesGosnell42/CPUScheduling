#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "algo.h"
#include "queue.h"

enum event_type {
	EV_PROC_CPU_STOP = 0,
	EV_PROC_CPU_PREEMPTION,
	EV_PROC_CPU_START,
	EV_PROC_CPU_CS,
	EV_PROC_IO_STOP,
	EV_PROC_ARRIVAL,
};

typedef struct {
	unsigned time;        // Event time.
	char id;              // Process id associated with event.
	enum event_type type; // Event type.
	int burst;            // Current burst ID associated with event.
	int time_slice;		  // Time slice
} event_t;

int Q_event_cmp_rr(const void* lhs, const void* rhs) {
	const event_t *lhe = lhs, *rhe = rhs;
	int d_time = lhe->time - rhe->time;
	int d_type = (int) lhe->type - (int) rhe->type;
	int d_id = lhe->id - rhe->id;
	return d_time != 0 ? d_time : (d_type != 0 ? d_type : d_id);
}

typedef struct {
	char id; 		 // Ready queue process id.
	unsigned arrival;
	enum event_type type;
	int burst;       // Next burst ID.
	unsigned t_join; // When this process joined the ready queue. (turnaround)
	unsigned p_join; // When this process joined the ready queue. (wait)
	int time_spent;	 // Record time spent in CPU
} ready_t;

int Q_ready_cmp_rr(const void* lhs, const void* rhs) {
	const ready_t *lhg = lhs, *rhg = rhs;
	int d_arrival = lhg->arrival - rhg->arrival;
	int d_type = (int) lhg->type - (int) rhg->type;
	int d_id = lhg->id - rhg->id;

	return d_arrival != 0 ? d_arrival : (d_type != 0 ? d_type : d_id);
}

void print_ready_queue_rr(queue_t* q) {
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

#ifdef DEBUG_MODE
#define DALWAYS_PRINT 1
#else
#define DALWAYS_PRINT 0
#endif

#define print_event(t, always_print, str, Q) \
	do { \
		if (DALWAYS_PRINT || always_print || t < 10000) { \
			printf("time %ums: " str " ", t); \
			print_ready_queue_rr(Q); \
			printf("\n"); \
		} \
	} while (0)

#define printf_event(t, always_print, fmt, Q, ...) \
	do { \
		if (DALWAYS_PRINT || always_print || t < 10000) { \
			printf("time %ums: " fmt " ", t, __VA_ARGS__); \
			print_ready_queue_rr(Q); \
			printf("\n"); \
		} \
	} while (0)

algo_stat_t algo_rr(const args_t* args, process_t* procs) {
	algo_stat_t rr_stats = {0}, rr_counts = {0};

	queue_t *Q_event = make_queue(), *Q_ready = make_queue();
	queue_set_cmp(Q_event, Q_event_cmp_rr);
	queue_set_cmp(Q_ready, Q_ready_cmp_rr);

	ready_t* guesses = calloc(args->n, sizeof(ready_t));
	for (int i = 0; i < args->n; ++i) {
		for (int j = 0; j < procs[i].cpu_burst_ct; ++j){
			stat_avg_add(&rr_stats.t_burst, &rr_counts.t_burst, procs[i].cpu_bursts[j], procs[i].cpu_bound);
		}
		event_t* e = malloc(sizeof(event_t));
		*e = (event_t){.time = procs[i].arrival_time,
		               .id = procs[i].id,
		               .type = EV_PROC_ARRIVAL,
		               .burst = 0};
		queue_push(Q_event, e);

		guesses[i].id = procs[i].id;
		guesses[i].burst = 0;
	}

	unsigned int t = 0;
	print_event(t, 1, "Simulator started for RR", Q_ready);

	enum cpu_mode { CM_IDLE = 0, CM_BURST, CM_CS } cpu_mode = CM_IDLE;

	int rr_error = 0;
	while (rr_error == 0 && queue_peek(Q_event)) {
		event_t* e = queue_pop(Q_event);

		t = e->time;

		switch (e->type) {
		case EV_PROC_CPU_STOP: {
			stat_avg_add(&rr_stats.t_turn, &rr_counts.t_turn,
			        t - guesses[e->id - 'A'].t_join + args->Tcs / 2,
			        procs[e->id - 'A'].cpu_bound);
			int bursts_left = procs[e->id - 'A'].cpu_burst_ct - 1 - e->burst;
			if (bursts_left == 0) {
				printf_event(t, 1, "Process %c terminated", Q_ready, e->id);
				free(e);
			} else {
				printf_event(t, 0,
				             "Process %c completed a CPU burst; %d "
				             "burst%s to go",
				             Q_ready, e->id, bursts_left, bursts_left == 1 ? "" : "s");

				// Requeue IO burst completion.
				guesses[e->id - 'A'].time_spent = 0;
				e->time = t + args->Tcs / 2 + procs[e->id - 'A'].io_bursts[e->burst];
				e->type = EV_PROC_IO_STOP;
				queue_push(Q_event, e);

				printf_event(t, 0,
				             "Process %c switching out of CPU; blocking on I/O"
				             " until time %ums",
				             Q_ready, e->id, e->time);
			}

			// Simulate context switch.
			cpu_mode = CM_CS;
			event_t* e_out = malloc(sizeof(event_t));
			*e_out = (event_t){.id = '#', .time = t + args->Tcs / 2, .type = EV_PROC_CPU_CS};
			queue_push(Q_event, e_out);

			break;
		}
		case EV_PROC_CPU_PREEMPTION: {
			if (queue_peek(Q_ready) != NULL) {
				unsigned bursts_len = procs[e->id - 'A'].cpu_bursts[e->burst];
				guesses[e->id - 'A'].time_spent += args->Tslice;

				printf_event(t, 0, "Time slice expired; preempting process %c with %dms remaining", Q_ready, e->id, bursts_len);
				// queue_push(Q_ready, &guesses[e->id - 'A']);
				

				stat_pre_inc(&rr_stats, procs[e->id - 'A'].cpu_bound);
				cpu_mode = CM_CS;
				e->time = t + args->Tcs / 2;
				e->type = EV_PROC_CPU_CS;

				queue_push(Q_event, e);
			} else {
				print_event(t, 0, "Time slice expired; no preemption because ready queue is empty", Q_ready);
				guesses[e->id - 'A'].time_spent += args->Tslice; 
				unsigned burst_len = procs[e->id - 'A'].cpu_bursts[e->burst];

				if (burst_len > args->Tslice) {
					e->time = t + args->Tslice;
					e->type = EV_PROC_CPU_PREEMPTION; 
					procs[e->id - 'A'].cpu_bursts[e->burst] -= args->Tslice;
				} else {
					e->time = t + burst_len;
					e->type = EV_PROC_CPU_STOP;
					procs[e->id - 'A'].cpu_bursts[e->burst] -= burst_len;
				}
				queue_push(Q_event, e);

			}
			break;
		}
		case EV_PROC_CPU_START: {
			unsigned burst_len = procs[e->id - 'A'].cpu_bursts[e->burst];

			if (guesses[e->id - 'A'].time_spent != 0) {
					printf_event(t, 0,
					             "Process %c started using the "
					             "CPU "
					             "for remaining %ums of %ums burst",
					             Q_ready, e->id, burst_len,
					             burst_len + guesses[e->id - 'A'].time_spent);
				} else {
					printf_event(t, 0,
					             "Process %c started "
					             "using the CPU "
					             "for %ums burst",
					             Q_ready, e->id, burst_len);
				}

			// Set CPU_mode.
			cpu_mode = CM_BURST;

			// Set e time and type
			if (burst_len > args->Tslice) {
				e->time = t + args->Tslice;
				e->type = EV_PROC_CPU_PREEMPTION; 
				procs[e->id - 'A'].cpu_bursts[e->burst] -= args->Tslice;
			} else {
				e->time = t + burst_len;
				e->type = EV_PROC_CPU_STOP;
				procs[e->id - 'A'].cpu_bursts[e->burst] -= burst_len;
			}


			queue_push(Q_event, e);

			// Update statistics.;;
			stat_cs_inc(&rr_stats, procs[e->id - 'A'].cpu_bound);

			break;
		}
		case EV_PROC_IO_STOP: {
			// Add to ready queue.
			guesses[e->id - 'A'].arrival = t;
			guesses[e->id - 'A'].type = e->type;
			guesses[e->id - 'A'].burst = e->burst + 1;
			guesses[e->id - 'A'].t_join = t;
			guesses[e->id - 'A'].p_join = t;
			queue_push(Q_ready, &guesses[e->id - 'A']);
			printf_event(t, 0,
			             "Process %c completed I/O; "
			             "added to ready queue",
			             Q_ready, e->id);
			free(e);
			break;
		}
		case EV_PROC_CPU_CS: {

			if (e->id != '#') {
				guesses[e->id - 'A'].arrival = t;
				guesses[e->id - 'A'].type = e->type;
				guesses[e->id - 'A'].p_join = t;
				queue_push(Q_ready, &guesses[e->id - 'A']);
			}

			cpu_mode = CM_IDLE;

			free(e);

			break;
		}
		case EV_PROC_ARRIVAL: {
			// Add to ready queue.
			guesses[e->id - 'A'].arrival = t;
			guesses[e->id - 'A'].type = e->type;
			guesses[e->id - 'A'].burst = 0;
			guesses[e->id - 'A'].t_join = t;
			guesses[e->id - 'A'].p_join = t;
			queue_push(Q_ready, &guesses[e->id - 'A']);
			printf_event(t, 0, "Process %c arrived; added to ready queue", Q_ready,
			             e->id);
			free(e);
			break;
		}
		default:
			fprintf(stderr, "ERROR: invalid event type %d.\n", e->type);
			rr_error = 1;
			break;
		}

		if (cpu_mode == CM_IDLE && !(queue_peek(Q_event) != NULL && ((event_t*)queue_peek(Q_event))->time == t)) {
			// Add first Q_ready as CPU burst start to Q_event.
			ready_t* r = queue_pop(Q_ready);
			if (r) {
				event_t* e_start = malloc(sizeof(event_t));
				*e_start = (event_t){.time = t + args->Tcs / 2,
				                     .id = r->id,
				                     .type = EV_PROC_CPU_START,
				                     .burst = r->burst};
				queue_push(Q_event, e_start);
				cpu_mode = CM_CS;

				if (r->time_spent != 0) {
					stat_avg_add(&rr_stats.t_wait, NULL , t - r->p_join,
				             procs[r->id - 'A'].cpu_bound);
				} else {
					stat_avg_add(&rr_stats.t_wait, &rr_counts.t_wait, t - r->p_join,
				             procs[r->id - 'A'].cpu_bound);
				}
			}
		}
	}

	print_event(t, 1, "Simulator ended for RR", Q_ready);

	free_queue(&Q_ready);
	free_queue(&Q_event);
	free(guesses);

	stat_calc_final(&rr_stats, &rr_counts, t);

	return rr_stats;
}
