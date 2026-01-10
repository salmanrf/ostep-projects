#ifndef _EXEC_
#define _EXEC_
#include <unistd.h>
#include <sys/wait.h>

#define BASE_EXECUTABLE "/bin"

int run_cd(char *cmd, int argc, LIST_NODE *args);

int run_path(LIST_NODE **PATHS, char *cmd, int argc, LIST_NODE *args);

int run_proc(LIST_NODE *PATHS, char *cmd, int argc, LIST_NODE *args);

void run_exit(char *cmd, int argc, LIST_NODE *args);

#endif
