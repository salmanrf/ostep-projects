#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "exec.h"

int run_all(PROGS *progs, LIST_NODE **PATHS) {
    LIST_NODE *prompts = progs->prompts;
    int progc = progs->progc;
    LIST_NODE *node = prompts;
    int progi = 0;
    int pids[progc];
    while(node != NULL) {
        char *prompt = (char *) node->data;
        ARGS *args = build_args(prompt, strlen(prompt));
        if(args == NULL) {
            node = node->next;
            continue;
        }
        char *cmd = args->cmd;

        int pid = 0;

//        print_ll_string(args->args);

        if(strcmp(cmd, "exit") == 0) {
            run_exit(args);
        } else if(strcmp(cmd, "cd") == 0) {
            run_cd(args);
        } else if(strcmp(cmd, "path") == 0) {
            run_path(PATHS, args);
        } else {
            pid = run_proc(*PATHS, args);
        }

        pids[progi] = pid;
        progi += 1;

        free_ll(&args->args);
        free(args);

        node = node->next;
    }
    for(int i = 0; i < progc; i++) {
        int pid = pids[i];

 //       printf("pid: %d\n", pid);

        if(pid == 0) {
            continue;
        }

        if(pid < 0) {
            print_generic_err();
            continue;
        }

        int stat;
        waitpid(pid, &stat, WUNTRACED);
//        printf("process returned %d\n", stat);
        if(stat < 0) {
            print_generic_err();
        }
    }

    return 0;
}

void run_exit(ARGS *args) {
    if(args->argc > 0) {
        print_generic_err();
        exit(0);
    }        
    exit(0);
}

int run_cd(ARGS *args) {
    if(args->argc != 1) {
        print_generic_err();
        return 1;
    }

    char *path = (char *) args->args->data;
    int ret = chdir(path);
    if(ret != 0) {
        print_generic_err();
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
    LIST_NODE *nextarg = args->args;
    char *COMMAND = NULL;
    while(path != NULL && ok != 0) {
        int len = 2 + strlen((char *) path->data) + strlen(cmd);
        COMMAND = (char *) malloc(len);
        snprintf(COMMAND, len, "%s/%s", (char *) path->data, cmd);

        ok = access(COMMAND, X_OK);
        path = path->next;
    }

    if(ok != 0) {
        print_generic_err();
        free(COMMAND);
        return 0;
    }

    int newargc = args->argc + 1;
    int lastarg_i = newargc - 1;
    char *argv[newargc + 1];
    argv[0] = COMMAND;
    char *stdout_path = NULL;
    for(int i = 1; i < newargc; i++) {
        char *argx = (char *) nextarg->data;
        if(strcmp(argx, ">") == 0) {
            if(i == lastarg_i || (lastarg_i - i) > 1) {
                return -1;
            }

            stdout_path = (char *) nextarg->next->data;
            argv[i] = NULL;
            break;
        }

        argv[i] = argx;
        nextarg = nextarg->next;
    }
    argv[newargc] = NULL; 

    int pid = fork();
    if(pid == 0) {
        if(stdout_path != NULL) {
            close(STDOUT_FILENO);
            FILE *newstdout = fopen(stdout_path, "w+");
            if(newstdout == NULL) {
                return 1;
            }
        }
        execv(COMMAND, argv);
    } else {
        return pid;
    }

    return 0;
}
