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
    char *output_path;
} ARGS;

char *str_trim_left(char *str, size_t *len);

void free_ll(LIST_NODE **ll);

void print_ll_string(LIST_NODE *ll);

void print_err(char *cmd, char *errmsg);

void print_generic_err();

PROGS *build_progs(char *main_prompt, size_t len);

ARGS *build_args(char *input, size_t len);

#endif
