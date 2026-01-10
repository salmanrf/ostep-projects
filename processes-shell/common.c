#include <stdlib.h>
#include "common.h"

void free_ll(LIST_NODE **ll) {
    LIST_NODE *node = *ll;
    while(node != NULL) {
        LIST_NODE *temp = node->next;
        free(node);
        node = temp;
    }
    *ll = NULL;
}

void print_ll_string(LIST_NODE *ll) {
    LIST_NODE *node = ll;
    while(node != NULL) {
        printf("%s", (char *) node->data);
        node = node->next;
    }
    printf("\n");
}

void print_err(char *cmd, char *errmsg) {
    printf("wish: %s: %s\n", cmd, errmsg);
}
