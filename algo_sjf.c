#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "algo.h"
#include "queue.h"

enum event_type {
	EV_PROC_CPU_STOP = 0,
	EV_PROC_CPU_START,
	EV_PROC_IO_STOP,
	EV_PROC_ARRIVAL,
	EV_PROC_CS_OUT
};

typedef struct {
	unsigned time;        // Event time.
	char id;              // Process id associated with event.
	enum event_type type; // Event type.
	int burst;            // Current burst ID associated with event.
} event_t;

int Q_event_cmp(const void* lhs, const void* rhs) {
	const event_t *lhe = lhs, *rhe = rhs;
	int d_time = (int) lhe->time - (int) rhe->time;
	int d_type = (int) lhe->type - (int) rhe->type;
	int d_id = (int) lhe->id - (int) rhe->id;
	return d_time != 0 ? d_time : (d_type != 0 ? d_type : d_id);
}

typedef struct {
	char id;         // Ready queue process id.
	unsigned tau;    // Estimated job time.
	int burst;       // Next burst ID.
	unsigned t_join; // Time when this process joined the ready queue.
} ready_t;

int Q_ready_cmp(const void* lhs, const void* rhs) {
	const ready_t *lhg = lhs, *rhg = rhs;
	int d_tau = lhg->tau - rhg->tau;
	return d_tau == 0 ? lhg->id - rhg->id : d_tau;
}

void print_ready_queue(queue_t* q) {
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
			print_ready_queue(Q); \
			printf("\n"); \
		} \
	} while (0)

#define printf_event(t, always_print, fmt, Q, ...) \
	do { \
		if (DALWAYS_PRINT || always_print || t < 10000) { \
			printf("time %ums: " fmt " ", t, __VA_ARGS__); \
			print_ready_queue(Q); \
			printf("\n"); \
		} \
	} while (0)

algo_stat_t algo_sjf(const args_t* args, process_t* procs) {
	algo_stat_t sjf_stats = {0}, sjf_counts = {0};
	queue_t *Q_event = make_queue(), *Q_ready = make_queue();
	queue_set_cmp(Q_event, Q_event_cmp);
	queue_set_cmp(Q_ready, Q_ready_cmp);

	ready_t* guesses = calloc(args->n, sizeof(ready_t));
	for (int i = 0; i < args->n; ++i) {
		event_t* e = malloc(sizeof(event_t));
		*e = (event_t){.time = procs[i].arrival_time,
		               .id = procs[i].id,
		               .type = EV_PROC_ARRIVAL,
		               .burst = 0};
		queue_push(Q_event, e);

		guesses[i].id = procs[i].id;
		guesses[i].tau = ceil(1 / args->lambda);
		guesses[i].burst = 0;
	}

	unsigned int t = 0;
	print_event(t, 1, "Simulator started for SJF", Q_ready);

	enum cpu_mode { CM_IDLE = 0, CM_CS, CM_BURST } cpu_mode = CM_IDLE;

	int sjf_error = 0;
	while (sjf_error == 0 && queue_peek(Q_event)) {
		event_t* e = queue_pop(Q_event);

		assert(e->time >= t);
		t = e->time;

		switch (e->type) {
		case EV_PROC_CPU_STOP: {
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
				guesses[e->id - 'A'].tau = exp_avg_tau(args->alpha, burst_len, tau_n);
				printf_event(t, 0,
				             "Recalculating tau for process %c: old tau %ums"
				             " ==> new tau %ums",
				             Q_ready, e->id, tau_n, guesses[e->id - 'A'].tau);

				// Requeue IO burst completion.
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
			*e_out = (event_t){.time = t + args->Tcs / 2, .type = EV_PROC_CS_OUT};
			queue_push(Q_event, e_out);

			break;
		}
		case EV_PROC_CS_OUT: {
			cpu_mode = CM_IDLE;
			free(e);

			break;
		}
		case EV_PROC_CPU_START: {
			unsigned burst_len = procs[e->id - 'A'].cpu_bursts[e->burst];
			// Print Process C (tau 1000ms) started using the CPU for 2920ms burst
			printf_event(t, 0,
			             "Process %c (tau %ums) started using the CPU "
			             "for %ums burst",
			             Q_ready, e->id, guesses[e->id - 'A'].tau, burst_len);

			// Set CPU_mode.
			cpu_mode = CM_BURST;

			// Queue CPU burst completion.
			e->time = t + burst_len;
			e->type = EV_PROC_CPU_STOP;
			queue_push(Q_event, e);

			// Update stats.
			stat_avg_add(&sjf_stats.t_burst, &sjf_counts.t_burst, burst_len,
			             procs[e->id - 'A'].cpu_bound);

			// At this time, we have done the context switch in, so turnaround is wait
			// time + burst + switch out.
			double t_turn =
			    t - guesses[e->id - 'A'].t_join + burst_len + args->Tcs / 2;
			stat_avg_add(&sjf_stats.t_turn, &sjf_counts.t_turn, t_turn,
			             procs[e->id - 'A'].cpu_bound);

			break;
		}
		case EV_PROC_IO_STOP: {
			// Add to ready queue.
			guesses[e->id - 'A'].burst = e->burst + 1;
			guesses[e->id - 'A'].t_join = t;
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
			queue_push(Q_ready, &guesses[e->id - 'A']);
			printf_event(t, 0, "Process %c (tau %ums) arrived; added to ready queue",
			             Q_ready, e->id, guesses[e->id - 'A'].tau);
			free(e);
			break;
		}
		default:
			fprintf(stderr, "ERROR: invalid event type %d.\n", e->type);
			sjf_error = 1;
			break;
		}

		if (cpu_mode == CM_IDLE) {
			// If there is a process in the ready queue, context switch it in.
			ready_t* r = queue_pop(Q_ready);
			if (r) {
				event_t* e_start = malloc(sizeof(event_t));
				*e_start = (event_t){.time = t + args->Tcs / 2,
				                     .id = r->id,
				                     .type = EV_PROC_CPU_START,
				                     .burst = r->burst};
				queue_push(Q_event, e_start);
				cpu_mode = CM_CS;

				stat_avg_add(&sjf_stats.t_wait, &sjf_counts.t_wait, t - r->t_join,
				             procs[r->id - 'A'].cpu_bound);

				stat_cs_inc(&sjf_stats, procs[r->id - 'A'].cpu_bound);
			}
		}
	}

	print_event(t, 1, "Simulator ended for SJF", Q_ready);

	free_queue(&Q_ready);
	free_queue(&Q_event);
	free(guesses);

	stat_calc_final(&sjf_stats, &sjf_counts, t);

	return sjf_stats;
}
