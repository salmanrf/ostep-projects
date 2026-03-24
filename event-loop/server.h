#include <poll.h>
#include "common.h"

#define INIT_POLLFDS_SIZE 10
#define POLL_TIMEOUT_MS 5000

void handle_conn(int client_fd, struct sockaddr *client_addr, int addrsize);

void handle_connected(int client_fd);

int handle_received_from_client(int client_fd);

struct pollfd *build_pollfds(int numfds, int startfd, short events);
