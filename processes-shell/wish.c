#include <stdio.h>
#include "common.h"
#include "exec.h"

int start_interactive();
int run_line(LIST_NODE **PATHS, char *line, ssize_t len);

int main(int argc, char *argv[]) {
    if(argc == 1) {
        start_interactive();
	}

	return 0;
}

int start_interactive() {
	char *line = NULL;
	size_t size = 0;
    ssize_t nread = 0;

    LIST_NODE *PATHS = (LIST_NODE *) malloc(sizeof(LIST_NODE));
    PATHS->data = BASE_EXECUTABLE;
    PATHS->next = NULL;

	do {
		if(line != NULL) {
            run_line(&PATHS, line, nread);
        } 
        printf("wish> ");
	} while((nread = getline(&line, &size, stdin)) != EOF);

	return 0;
}

int run_line(LIST_NODE **PATHS, char *input, ssize_t len) {
    if(len == 0) {
        return 0;
    }

    char *prompt = strdup(input);
    ARGS *args = build_args(prompt, len);
    char *cmd = args->cmd;

    int ret = 0;
    if(strcmp(cmd, "exit") == 0) {
        run_exit(args);
    } else if(strcmp(cmd, "cd") == 0) {
        ret = run_cd(args);
    } else if(strcmp(cmd, "path") == 0) {
        ret = run_path(PATHS, args);
    } else {
        ret = run_proc(*PATHS, args);
    }

    free_ll(&args->args);
    free(args);
    return ret;
}
