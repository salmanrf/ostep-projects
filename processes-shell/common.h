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

typedef struct PROGS {
    int progc;
    LIST_NODE *prompts;
} PROGS;

typedef struct ARGS {
    char *cmd;
    LIST_NODE *args;
    int argc;
} ARGS;

char *str_trim_left(char *str, int *len);

void free_ll(LIST_NODE **ll);

void print_ll_string(LIST_NODE *ll);

void print_err(char *cmd, char *errmsg);

PROGS *build_progs(char *main_prompt, ssize_t len);

ARGS *build_args(char *input, ssize_t len);

#endif
