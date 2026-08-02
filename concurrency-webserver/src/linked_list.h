#ifndef __LINKED_LIST__
#define __LINKED_LIST__

#include <stdlib.h>

typedef struct node {
	void *data;
	struct node *prev;
	struct node *next;
} node_t;

typedef struct linked_list {
	node_t *head;
	node_t *tail;
	node_t *size;
} linked_list_t;

linked_list_t *list_create();

node_t *take_first(linked_list_t *list);

#endif