#include <stdio.h>
#include "common.h"
#include "exec.h"

int start_interactive(LIST_NODE **PATHS);
int start_batch(LIST_NODE **PATHS, char *input_path);
int run_line(LIST_NODE **PATHS, char *line, ssize_t len);

int main(int argc, char *argv[]) {
    LIST_NODE *PATHS = (LIST_NODE *) malloc(sizeof(LIST_NODE));
    PATHS->data = BASE_EXECUTABLE;
    PATHS->next = NULL;

    // Too many argument
    if(argc > 2) {
        print_generic_err();
        return 1;
    }

    // Interactive mode
    if(argc == 1) {
        return start_interactive(&PATHS);
	}

    // Batch mode
	return start_batch(&PATHS, argv[1]);
}

int start_batch(LIST_NODE **PATHS, char *input_path) {
    FILE *input_f = fopen(input_path, "r");
    if(input_f == NULL) {
        print_generic_err();
        return 1;
    }

    char *line = NULL;
	size_t size = 0;
    ssize_t nread = 0;
    while((nread = getline(&line, &size, input_f)) != EOF) {
        run_line(PATHS, line, nread);
    }

    return 0;
}

int start_interactive(LIST_NODE **PATHS) {
	char *line = NULL;
	size_t size = 0;
    ssize_t nread = 0;

	do {
		if(line != NULL) {
            run_line(PATHS, line, nread);
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
    PROGS *progs = build_progs(prompt, len);

    int ret = run_all(progs, PATHS);

    return ret;
}
