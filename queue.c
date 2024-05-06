#include "queue.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct queue {
	void** data;
	size_t size;
	size_t cap;
	queue_cmp cmp;
};

int queue_default_cmp(const void* lhs, const void* rhs) {
	return (char*) lhs - (char*) rhs;
}

#define QUEUE_INITIAL_SIZE 32

queue_t* make_queue(void) {
	queue_t* q = malloc(sizeof(queue_t));
	q->data = calloc(QUEUE_INITIAL_SIZE, sizeof(void*));
	q->size = 0;
	q->cap = QUEUE_INITIAL_SIZE;
	q->cmp = queue_default_cmp;
	return q;
}

void free_queue(queue_t** q) {
	if (*q != NULL) {
		free((*q)->data);
		free(*q);
		*q = NULL;
	}
}

void queue_set_cmp(queue_t* q, queue_cmp cmp) {
	assert(q);
	assert(cmp);
	q->cmp = cmp;
}

int heap_child_left(int i) { return i * 2 + 1; }

int heap_child_right(int i) { return i * 2 + 2; }

int heap_parent(int i) { return (i - 1) / 2; }

// percolate up. return new i.
size_t percolate_up(queue_t* q, size_t i) {
	assert(q);
	assert(i < q->size);

	int p = heap_parent(i);
	while (i > 0 && q->cmp(q->data[i], q->data[p]) < 0) {
		// swap elements
		void* tmp = q->data[p];
		q->data[p] = q->data[i];
		q->data[i] = tmp;

		i = p;
		p = heap_parent(i);
	}

	return i;
}

// percolate down. return new i.
size_t percolate_down(queue_t* q, size_t i) {
	assert(q);
	assert(i < q->size);

	size_t l = heap_child_left(i), r = heap_child_right(i);
	l = l < q->size ? l : i;
	r = r < q->size ? r : i;
	size_t c = q->cmp(q->data[l], q->data[r]) < 0 ? l : r;
	while (i < q->size && c < q->size && q->cmp(q->data[i], q->data[c]) > 0) {
		// swap elements
		void* tmp = q->data[c];
		q->data[c] = q->data[i];
		q->data[i] = tmp;

		i = c;
		l = heap_child_left(i);
		r = heap_child_right(i);
		l = l < q->size ? l : i;
		r = r < q->size ? r : i;
		c = q->cmp(q->data[l], q->data[r]) < 0 ? l : r;
	}

	return i;
}

void queue_push(queue_t* q, void* v) {
	assert(q);
	assert(v);

	// Potentially increase storage.
	if (q->size == q->cap) {
		q->cap *= 2;
		q->data = realloc(q->data, q->cap * sizeof(void*));
	}

	// Add to end.
	q->data[q->size] = v;
	++q->size;

	// Percolate up.
	percolate_up(q, q->size - 1);
}

void* queue_pop(queue_t* q) {
	assert(q);

	// Special cases.
	if (q->size == 0) {
		return NULL;
	} else if (q->size == 1) {
		--q->size;
		return q->data[0];
	}

	// swap head with end.
	void* tmp = q->data[q->size - 1];
	q->data[q->size - 1] = q->data[0];
	q->data[0] = tmp;

	q->size--;

	percolate_down(q, 0);

	// return end.
	return q->data[q->size];
}

void* queue_peek(queue_t* q) {
	assert(q);

	if (q->size == 0) return NULL;

	return q->data[0];
}

void queue_update(queue_t* q, void* v) {
	assert(q);
	assert(v);

	int i = queue_search(q, v);

	i = percolate_up(q, i);
	i = percolate_down(q, i);
}

// Search without using cmp function.
size_t queue_search(queue_t* q, void* v) {
	assert(q);
	for (size_t i = 0; i < q->size; ++i) {
		if (q->data[i] == v) { return i; }
	}

	return -1;
}

void queue_delete(queue_t* q, size_t i) {
	assert(q);
	assert(i < q->size);

	// Swap to end.
	void* tmp = q->data[q->size - 1];
	q->data[q->size - 1] = q->data[i];
	q->data[i] = tmp;

	q->size--;

	if (q->size != i) {
		i = percolate_up(q, i);
		i = percolate_down(q, i);
	}
}

queue_t* queue_copy(queue_t* dst, const queue_t* src) {
	assert(src);
	assert(dst);

	dst->size = src->size;
	dst->cap = src->cap;
	dst->cmp = src->cmp;
	dst->data = realloc(dst->data, dst->cap * sizeof(void*));

	for (size_t i = 0; i < dst->size; ++i) { dst->data[i] = src->data[i]; }

	return dst;
}
