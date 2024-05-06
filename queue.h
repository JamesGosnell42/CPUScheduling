#include <stddef.h>

#ifndef OPSYS_SIM_QUEUE_H_
#define OPSYS_SIM_QUEUE_H_

typedef struct queue queue_t;

/**
 * Allocate a new queue object.
 * @return the new queue obbject.
 */
queue_t* make_queue(void);

/**
 * Free the memory associated with the queue object.
 * Also sets the q value to NULL. Values stored by q
 * are not freed -- only the container holding them..
 */
void free_queue(queue_t** q);

/**
 * queue_cmp is a function that compares items in the queue. It should return
 * < 0 if the left item goes before the right item, > 0 if the left item goes
 * after the right item, and 0 if they compare equal. The default function
 * sorts by pointer values.
 */
typedef int (*queue_cmp)(const void*, const void*);

/**
 * Set the queue comparison function.
 * @pre: q != NULL && cmp != NULL.
 */
void queue_set_cmp(queue_t* q, queue_cmp cmp);

/**
 * Add an item to the queue. If the item v compares equal to another item in
 * the queue, replace the old item with the new item.
 */
void queue_push(queue_t* q, void* v);

/**
 * Remove the item at the head of the queue.
 * @returns The pointer to the value or NULL if the queue is empty.
 */
void* queue_pop(queue_t* q);

/**
 * Peek at the head of the queue without removal.
 * @returns The value at the head of the queue or NULL if empty.
 */
void* queue_peek(queue_t* q);

/**
 * Search the queue for item v without using cmp.
 * @returns An iterator to item v or -1 if not found.
 */
size_t queue_search(queue_t* q, void* v);

/**
 * Remove item i from the queue (used with queue_search).
 */
void queue_delete(queue_t* q, size_t i);

/**
 * Copy queue src contents into dst. Both must have been created using
 * `make_queue()`.
 */
queue_t* queue_copy(queue_t* dst, const queue_t* src);

#endif // OPSYS_SIM_QUEUE_H_
