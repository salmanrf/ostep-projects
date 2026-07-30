#ifndef __QUEUE_H__
#define __QUEUE_H__

typedef struct queue_node {
	void *data;
	struct queue_node *next;
} queue_node_t;

typedef struct queue {
	queue_node_t *head;
	queue_node_t *tail;
	int size;
} queue_t;

queue_t *queue_create(void);
int queue_enqueue(queue_t *q, void *data);
void *queue_dequeue(queue_t *q);
void *queue_peek(queue_t *q);
int queue_size(queue_t *q);
int queue_is_empty(queue_t *q);
void queue_destroy(queue_t *q);

#endif // __QUEUE_H__
