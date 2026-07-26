#include <poll.h>
#include <aio.h>
#include "common.h"
#include "state.h"

#define MAX_CONNECTION 10 
#define POLL_TIMEOUT_MS 3000
#define SERVER_READ_BUF_SIZE 1024 * 1024 

int handle_new_connection(STATE_STORE *state, struct pollfd *pfds, struct pollfd *pfd);
int handle_disconnected(STATE_STORE *state, struct pollfd *pfds, struct pollfd *pfd);
int handle_client_input(STATE_STORE *state, struct pollfd *pfds, struct pollfd *pfd);
int handle_asset_input(STATE_STORE *state, struct pollfd *pfds, struct pollfd *pfd);

struct pollfd *build_pollfds(int numfds, int startfd, short events);

typedef enum {
  NEW_CONNECTION, 
  CLIENT_IN_READY, 
  ASSET_IN_READY,
  CLIENT_OUT_READY, 
  DISCONNECTED,
  EMPTY,
} EEvents; 

EEvents get_event(STATE_STORE *store, int server_fd, struct pollfd *fd);
