#include <stdlib.h>
#include "queue.h"
#include "request.h"

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
	node->prev = q->tail;
	node->next = NULL;

	if (q->tail == NULL) {
		q->head = node;
		q->tail = node;
		q->size += 1;

		return 0;
	}

	queue_node_t *q_node = node->prev;
	Request *n_req = ((Request *) data);
	Request *q_req = ((Request *) q_node->data);

	while(q_node != NULL) {
		int q_filesize = q_req->filestat->st_size;
		int n_filesize = n_req->filestat->st_size;
		if(n_filesize < q_filesize) {
			q_node = q_node->prev;
		}
		break;
	}

	// * Reached head, becoming new head
	if(q_node == NULL) {
		node->prev = NULL;
		node->next = q->head;
		q->head = node;

		return 0;
	}

	queue_node_t *old_next = q_node->next;
	q_node->next = node;
	node->next = old_next;

	return 0;
}

// * Returns the front element's data, or NULL if the queue is empty
void *queue_dequeue(queue_t *q) {
	if (q->head == NULL) {
		return NULL;
	}

	queue_node_t *node = q->head;
	void *data = node->data;

	queue_node_t *next_head = node->next;
	
	q->head = next_head;
	if (q->head == NULL) {
		// * Queue becomes empty
		q->tail = NULL;
	} else {
		// * Queue not empty, first node has no prev
		next_head->prev = NULL;
	}
	q->size -= 1;

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
