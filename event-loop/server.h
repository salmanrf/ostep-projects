#include "common.h"

void handle_conn(int client_fd, struct sockaddr *client_addr, int addrsize);

void handle_connected(int client_fd);

int handle_received_from_client(int client_fd);
