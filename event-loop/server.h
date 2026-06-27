#include <poll.h>
#include <aio.h>
#include "common.h"
#include "state.h"

#define MAX_CONNECTION 10 
#define POLL_TIMEOUT_MS 500
#define SERVER_READ_BUF_SIZE 1024 * 1024 

void handle_conn(int client_fd, struct sockaddr *client_addr, int addrsize);

void handle_connected(int client_fd);

void handle_new_connection(STATE_STORE *state_store, int conn_fd);

int handle_input(STATE_STORE *ss, int client_fd);

struct pollfd *build_pollfds(int numfds, int startfd, short events);


