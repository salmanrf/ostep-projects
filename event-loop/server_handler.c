#include "server.h"

EEvents get_event(STATE_STORE *store, int server_fd, struct pollfd *fd) {
    if(fd == NULL) {
        return -1;
    }
    
    if(fd->revents & POLLHUP) {
        return DISCONNECTED;
    }

    if(fd->revents & POLLIN) {
        if(server_fd == fd->fd) {
            return NEW_CONNECTION;
        }
        
        REQ_STATE *state = get_state(store, fd->fd);
        if(state->type == STATE_CLIENT) {
            return CLIENT_IN_READY;
        } 
        if(state->type == STATE_ASSET) {
            return ASSET_IN_READY;
        }
    }
    
    return EMPTY;
}

int handle_new_connection(STATE_STORE *store, struct pollfd *_, struct pollfd *pfd) {
    if(store->current_n_conns + 1 > MAX_CONNECTION) {
        pfd->fd = -1 * abs(pfd->fd);
        return -1;
    }

    struct pollfd *server = pfd;
    int client_fd;
    struct sockaddr client_addr;
    socklen_t addrsize = sizeof client_addr; 
    if((client_fd = accept(server->fd, &client_addr, &addrsize)) < 0) {
        perror("accept: ");
        return -1;
    }
    store->current_n_conns += 1;

    REQ_STATE *state = new_state(client_fd, STATE_CLIENT); 
    set_state(store, client_fd, state);

    return client_fd;
}

int handle_disconnected(STATE_STORE *state, struct pollfd *_, struct pollfd *pfd) {
    close(pfd->fd);
    pfd->fd = -1 * abs(pfd->fd);
    state->current_n_conns -= 1;
    return 0;
}

int handle_client_input(STATE_STORE *store, struct pollfd *_, struct pollfd *pfd) {
    int client_fd = pfd->fd;
    REQ_STATE *state = get_state(store, client_fd);
    if(state == NULL) {
        fprintf(stderr, "Invalid state: not initialized\n");
        return -1;
    }

    if(state->status != WAITING_INPUT) {
        fprintf(stderr, "Unexpected input, ignoring...\n");
        return -1;
    }
    
    char path[200] = "assets/";
    int max_filename_len = 100;
    char recv_buf[max_filename_len];
    int received;

    if((received = recv(client_fd, recv_buf, 100, 0)) == 0) {
        return -1;
    }

    strncat(path, recv_buf, received);
    recv_buf[received] = '\0';

    printf("Opening: '%s'\n", path);

    int fd = open(path, O_RDONLY | O_NONBLOCK, 0);
    if(fd < 0) {
        perror("open error");
        return -1; 
    }

    // * Initialize new state to track requested file for streaming
    REQ_STATE *asset_state = new_state(fd, STATE_ASSET); 
    asset_state->parent = state; 
    asset_state->status = WAITING_INPUT;
    set_state(store, fd, asset_state);
    state->status = PROCESSING;

    struct aiocb *aioctl = malloc(sizeof(struct aiocb));
    char *read_buf = (char *) malloc(SERVER_READ_BUF_SIZE);
    aioctl->aio_fildes = fd;
    aioctl->aio_buf = read_buf;
    aioctl->aio_nbytes = SERVER_READ_BUF_SIZE;
    if(aio_read(aioctl) < 0) {
        perror("aio_read: ");
        return -1;
    }

    asset_state->aioctl = aioctl;

    return fd; 
}

int handle_asset_input(STATE_STORE *store, struct pollfd *_, struct pollfd *pfd) {
    int client_fd = pfd->fd;
    REQ_STATE *state = get_state(store, client_fd);
    if (state == NULL) {
        return -1;
    }

    char *read_buf = (char *) state->aioctl->aio_buf;

    int stat = aio_error(state->aioctl);
    if(stat != 0) {
        printf("aio_read not ready\n");
        return 0;
    }
    int nread = aio_return(state->aioctl);
    if(nread <= 0) {
        printf("reached EOF, closing fd...\n");
        close(state->aioctl->aio_fildes); 
        return -1;
    }
    
    read_buf[nread] = '\0';
    printf("aio_read success: read %d bytes\n", nread);

    int parent_fd = state->parent->conn_fd;

    printf("Writing to %d bytes to socket: %d\n", nread, parent_fd);
    if(write(parent_fd, read_buf, nread) < 0) {
        perror("write: ");
        return -1;
    }

    // * Read more file content
    state->aioctl->aio_offset += nread; 
    if(aio_read(state->aioctl) < 0) {
        perror("aio_read: ");
        return -1;
    }

    return 0;
}

struct pollfd *build_pollfds(int numfds, int startfd, short events) {
    struct pollfd *pollfds = calloc(numfds, sizeof(struct pollfd));

    for(int i = 0; i < numfds; i++) {
        pollfds[i].fd = -1 * (startfd + i);
        pollfds[i].events = events;
    }

    return pollfds;
}