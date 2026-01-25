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

typedef struct ARGS {
    char *cmd;
    LIST_NODE *args;
    int argc;
} ARGS;

void free_ll(LIST_NODE **ll);

void print_ll_string(LIST_NODE *ll);

void print_err(char *cmd, char *errmsg);

ARGS *build_args(char *input, ssize_t len);

#endif
