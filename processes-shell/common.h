#ifndef _COMMON_
#define _COMMON_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct LIST_NODE {
    struct LIST_NODE *next;
    void *data;
} LIST_NODE;

void free_ll(LIST_NODE **ll);

void print_ll_string(LIST_NODE *ll);

void print_err(char *cmd, char *errmsg);

#endif
