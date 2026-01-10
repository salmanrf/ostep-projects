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

    int ret = 0;

    LIST_NODE *PATHS = (LIST_NODE *) malloc(sizeof(LIST_NODE));
    PATHS->data = BASE_EXECUTABLE;
    PATHS->next = NULL;

	do {
		if(line != NULL) {
            ret = run_line(&PATHS, line, nread);
        }

        if(ret != 0) {
            break;
        } else {
            printf("wish> ");
        }

	} while((nread = getline(&line, &size, stdin)) != EOF);

    free(line);

	return 0;
}

int run_line(LIST_NODE **PATHS, char *input, ssize_t len) {
    if(len == 0) {
        return 0;
    }

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
        if(strcmp(token, " ") == 0) {
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

    int ret = 0;
    if(strcmp(cmd, "exit") == 0) {
        run_exit(cmd, argc, args);
    } else if(strcmp(cmd, "cd") == 0) {
        ret = run_cd(cmd, argc, args);
    } else if(strcmp(cmd, "path") == 0) {
        ret = run_path(PATHS, cmd, argc, args);
    } else {
        ret = run_proc(*PATHS, cmd, argc, args);
    }

    free_ll(&args);
    return ret;
}
