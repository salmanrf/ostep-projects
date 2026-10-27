#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

typedef struct list_node {
	void *data;
	struct list_node *next;
	struct list_node *prev;
} list_node_t;

typedef struct linked_list {
	list_node_t *head;
	list_node_t *tail;
	int size;
} linked_list_t;

linked_list_t *list_create(void);
int list_push_front(linked_list_t *l, void *data);
int list_push_back(linked_list_t *l, void *data);
void *list_pop_front(linked_list_t *l);
void *list_pop_back(linked_list_t *l);
void *list_peek_front(linked_list_t *l);
void *list_peek_back(linked_list_t *l);
void *list_remove(linked_list_t *l, list_node_t *node);
int list_size(linked_list_t *l);
int list_is_empty(linked_list_t *l);
void list_destroy(linked_list_t *l);

#endif
