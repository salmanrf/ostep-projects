#include <stdlib.h>
#include "common.h"

ARGS *build_args(char *input, ssize_t len) {
    char *prompt = strdup(input);

    if(prompt[len - 1] == '\n') {
        prompt[len - 1] = '\0';
    }

    char *cmd = NULL;
    if(!(cmd = strsep(&prompt, " "))) {
        return 0;     
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
