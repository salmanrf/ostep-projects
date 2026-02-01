#ifndef _EXEC_
#define _EXEC_
#include <unistd.h>
#include <sys/wait.h>

#define BASE_EXECUTABLE "/bin"

int run_cd(ARGS *args);

int run_path(LIST_NODE **PATHS, ARGS *args);

int run_proc(LIST_NODE *PATHS, ARGS *args);

int run_all(PROGS *progs, LIST_NODE **PATHS);

void run_exit(ARGS *args);

#endif
