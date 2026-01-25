#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "exec.h"

void run_exit(ARGS *args) {
    if(args->argc > 0) {
        print_err(args->cmd, "too many arguments");
        exit(1);
    }        
    exit(0);
}

int run_cd(ARGS *args) {
    if(args->argc == 0) {
        return 0;
    }

    if(args->argc > 1) {
        print_err(args->cmd, "too many arguments");
        return 1;
    }

    char *path = (char *) args->args->data;
    int ret = chdir(path);
    if(ret != 0) {
        char *msg = ": No such file or directory";
        int len = strlen(path) + strlen(msg);
        char *errmsg = (char *) malloc(len);
        snprintf(errmsg, len, "%s%s", (char *) path, msg);
        print_err(args->cmd, errmsg);
        return ret;
    }

    return 0;
}

int run_path(LIST_NODE **dest, ARGS *args) {
    if(args->argc == 0) {
        *dest = NULL;
    }

    LIST_NODE *new_paths = NULL; 
    LIST_NODE *node = args->args;
    while(node != NULL) {
        LIST_NODE *arg = (LIST_NODE *) malloc(sizeof(LIST_NODE));
        arg->data = node->data;
        arg->next = NULL;

        if(new_paths == NULL) {
            new_paths = arg;
        } else {
            new_paths->next = arg;
        }

        node = node->next;
    }

    *dest = new_paths;
    
    return 0;
}

int run_proc(LIST_NODE *PATHS, ARGS *args) {
    LIST_NODE *path = PATHS;

    int ok = -1;
    char *cmd = args->cmd;
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
        int newargc = args->argc + 1;
        char *argv[newargc];
        argv[0] = COMMAND;
        LIST_NODE *arg = args->args;
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
