#include "linked_list.h"
#include <stdlib.h>

linked_list_t *list_create(void) {
	linked_list_t *l = malloc(sizeof(linked_list_t));
	if (l == NULL) {
		return NULL;
	}
	l->head = NULL;
	l->tail = NULL;
	l->size = 0;
	return l;
}

// * Returns 0 on success, -1 on allocation failure
int list_push_front(linked_list_t *l, void *data) {
	list_node_t *node = malloc(sizeof(list_node_t));
	if (node == NULL) {
		return -1;
	}
	node->data = data;
	node->prev = NULL;
	node->next = l->head;

	if (l->head == NULL) {
		// * List was empty, node becomes both head and tail
		l->tail = node;
	} else {
		l->head->prev = node;
	}
	l->head = node;
	l->size += 1;

	return 0;
}

// * Returns 0 on success, -1 on allocation failure
int list_push_back(linked_list_t *l, void *data) {
	list_node_t *node = malloc(sizeof(list_node_t));
	if (node == NULL) {
		return -1;
	}
	node->data = data;
	node->prev = l->tail;
	node->next = NULL;

	if (l->tail == NULL) {
		// * List was empty, node becomes both head and tail
		l->head = node;
	} else {
		l->tail->next = node;
	}
	l->tail = node;
	l->size += 1;

	return 0;
}

// * Unlinks and frees the node, returns its data; node must belong to l
void *list_remove(linked_list_t *l, list_node_t *node) {
	if (node == NULL) {
		return NULL;
	}

	if (node->prev == NULL) {
		// * Node is the head
		l->head = node->next;
	} else {
		node->prev->next = node->next;
	}

	if (node->next == NULL) {
		// * Node is the tail
		l->tail = node->prev;
	} else {
		node->next->prev = node->prev;
	}
	l->size -= 1;

	void *data = node->data;
	free(node);
	return data;
}

// * Returns the front element's data, or NULL if the list is empty
void *list_pop_front(linked_list_t *l) { return list_remove(l, l->head); }

// * Returns the back element's data, or NULL if the list is empty
void *list_pop_back(linked_list_t *l) { return list_remove(l, l->tail); }

void *list_peek_front(linked_list_t *l) {
	if (l->head == NULL) {
		return NULL;
	}
	return l->head->data;
}

void *list_peek_back(linked_list_t *l) {
	if (l->tail == NULL) {
		return NULL;
	}
	return l->tail->data;
}

int list_size(linked_list_t *l) { return l->size; }

int list_is_empty(linked_list_t *l) { return l->size == 0; }

// * Frees all nodes and the list itself; does not free the data pointers
void list_destroy(linked_list_t *l) {
	if (l == NULL) {
		return;
	}
	while (l->head != NULL) {
		list_pop_front(l);
	}
	free(l);
}
