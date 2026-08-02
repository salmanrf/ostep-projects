#include "linked_list.h"

linked_list_t *list_create() {
    linked_list_t *list = (linked_list_t *) malloc(sizeof(linked_list_t));  
    list->size = 0;

    return list;
}

node_t *take_first(linked_list_t *list) {
	if(list == NULL || list->head == NULL) {
		return NULL;
	}

	node_t *n = list->head;
	list->head = n->next;

	return n;
}