#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "exec.h"

void run_exit(char *cmd, int argc, LIST_NODE *args) {
    if(argc > 0) {
        print_err(cmd, "too many arguments");
        exit(1);
    }        
    exit(0);
}

int run_cd(char *cmd, int argc, LIST_NODE *args) {
    if(argc == 0) {
        return 0;
    }

    if(argc > 1) {
        print_err(cmd, "too many arguments");
        return 1;
    }

    char *path = (char *) args->data;
    int ret = chdir(path);
    if(ret != 0) {
        char *errmsg = ": No such file or directory";
        print_err(cmd, strcat(path, errmsg));
        return ret;
    }

    return 0;
}

int run_path(LIST_NODE **dest, char *cmd, int argc, LIST_NODE *args) {
    if(argc == 0) {
        *dest = NULL;
    }

    LIST_NODE *new_paths = NULL; 
    LIST_NODE *node = args;
    LIST_NODE *curr_cp = NULL;
    while(node != NULL) {
        LIST_NODE *arg = (LIST_NODE *) malloc(sizeof(LIST_NODE));
        arg->data = node->data;
        arg->next = node->next;

        if(new_paths == NULL) {
            new_paths = arg;
            curr_cp = arg;
        } else {
            curr_cp->next = arg;
        }

        node = node->next;
        curr_cp = arg;
    }

    *dest = new_paths;
    
    return 0;
}

int run_proc(LIST_NODE *PATHS, char *cmd, int argc, LIST_NODE *args) {
    LIST_NODE *path = PATHS;

    int ok = -1;
    char *COMMAND = NULL;
    while(path != NULL && ok != 0) {
        int len = 2 + strlen((char *) path->data) + strlen(cmd);
        COMMAND = (char *) malloc(len);
        snprintf(COMMAND, len, "%s/%s", (char *) path->data, cmd);

        ok = access(COMMAND, X_OK);
        path = path->next;
    }

    if(ok != 0) {
        print_err(cmd, "command not found");
        free(COMMAND);
        return 0;
    }

    int pid = fork();
    if(pid == 0) {
        int newargc = argc + 1;
        char *argv[newargc];
        argv[0] = COMMAND;
        LIST_NODE *arg = args;
        for(int i = 1; i < newargc; i++) {
            argv[i] = (char *) arg->data;
            arg = arg->next;
        }
        argv[newargc] = NULL; 
        execv(COMMAND, argv);
    } else {
        wait(NULL);
        free(COMMAND);
    }

    return 0;
}
