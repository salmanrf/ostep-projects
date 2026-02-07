#include <stdlib.h>
#include "common.h"

PROGS *build_progs(char *main_prompt, ssize_t len) {
    char *prompt = strdup(main_prompt);

    if(prompt[len - 1] == '\n') {
        prompt[len - 1] = '\0';
    }

    LIST_NODE *prompts = NULL;
    int progc = 0;
    char *token = NULL;

    LIST_NODE *node = NULL;
    while((token = strsep(&prompt, "&"))) {
        int len = strlen(token);
        token = str_trim_left(token, &len);
        if(!token || strlen(token) == 0) {
            continue;
        }

        LIST_NODE *prog = (LIST_NODE *) malloc(sizeof(LIST_NODE));
        prog ->data = strdup(token);
        prog ->next = NULL;

        if(prompts == NULL) {
            prompts = prog;
        } else {
            node->next = prog;
        }

        node = prog;
        progc += 1;
    }

    PROGS *ret = (PROGS *) malloc(sizeof(PROGS));
    ret->prompts = prompts;
    ret->progc = progc;

    return ret;

}

ARGS *build_args(char *input, ssize_t len) {
    char *prompt = strdup(input);

    if(prompt[len - 1] == '\n') {
        prompt[len - 1] = '\0';
    }

    char *cmd = NULL;
    if(!(cmd = strsep(&prompt, " ")) || strlen(cmd) == 0) {
        return NULL;     
    }

    LIST_NODE *args = NULL;
    int argc = 0;
    char *token = NULL;

    LIST_NODE *node = NULL;
    while((token = strsep(&prompt, " "))) {
        if(!token || strlen(token) == 0) {
            continue;
        }

        LIST_NODE *arg = (LIST_NODE *) malloc(sizeof(LIST_NODE));
        arg->data = strdup(token);
        arg->next = NULL;

        if(args == NULL) {
            args = arg;
        } else {
            node->next = arg;
        }

        node = arg;
        argc += 1;
    }

    ARGS *ret = (ARGS *) malloc(sizeof(ARGS));
    ret->cmd = cmd;
    ret->args = args;
    ret->argc = argc;

    return ret;
}

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

void print_generic_err() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char *str_trim_left(char *str, int *len) {
    int oldlen = *len;
    int start = 0;

    while(str[start] == ' ' || str[start] == '\t') {
        start += 1;
    }

    if(start == 0) {
        return str;
    }

    int newlen = (oldlen - start) + 1;

    char *newstr = malloc(newlen);
    int i = 0;
    for(; i < newlen; i++) {
        int orig_i = start + i;
        newstr[i] = str[orig_i];
    }
    newstr[i] = '\0';
    *len = newlen;

    return newstr;
}
