#include <stdlib.h>
#include "queue.h"

queue_t *queue_create(void) {
	queue_t *q = malloc(sizeof(queue_t));
	if (q == NULL) {
		return NULL;
	}
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
	return q;
}

// * Returns 0 on success, -1 on allocation failure
int queue_enqueue(queue_t *q, void *data) {
	queue_node_t *node = malloc(sizeof(queue_node_t));
	if (node == NULL) {
		return -1;
	}
	node->data = data;
	node->next = NULL;

	if (q->tail == NULL) {
		q->head = node;
		q->tail = node;
	} else {
		q->tail->next = node;
		q->tail = node;
	}
	q->size++;
	return 0;
}

// * Returns the front element's data, or NULL if the queue is empty
void *queue_dequeue(queue_t *q) {
	if (q->head == NULL) {
		return NULL;
	}

	queue_node_t *node = q->head;
	void *data = node->data;

	q->head = node->next;
	if (q->head == NULL) {
		q->tail = NULL;

	}
	q->size--;

	free(node);
	return data;
}

void *queue_peek(queue_t *q) {
	if (q->head == NULL) {
		return NULL;
	}
	return q->head->data;
}

int queue_size(queue_t *q) {
	return q->size;
}

int queue_is_empty(queue_t *q) {
	return q->size == 0;
}

// * Frees all nodes and the queue itself; does not free the data pointers
void queue_destroy(queue_t *q) {
	if (q == NULL) {
		return;
	}
	while (q->head != NULL) {
		queue_dequeue(q);
	}
	free(q);
}
