#include "server.h"

void handle_conn(int client_fd, struct sockaddr *client_addr, int addrsize) {
    time_t rawtime;
    struct tm *timeinfo;
    char datestr[100];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(datestr, sizeof(datestr), "%Y-%m-%d %H:%M:%S", timeinfo);
    printf("Formatted date string: %s\n", datestr);

    if(send(client_fd, datestr, strlen(datestr), 0) < 0) {
        perror("send error");
        close(client_fd);
    } 

    close(client_fd);
}

void handle_connected(int client_fd) {
    time_t rawtime;
    struct tm *timeinfo;
    char datestr[100];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(datestr, sizeof(datestr), "%Y-%m-%d %H:%M:%S", timeinfo);
    printf("Formatted date string: %s\n", datestr);

    if(send(client_fd, datestr, strlen(datestr), 0) < 0) {
        perror("send error");
        close(client_fd);
    } 
}

int handle_input(STATE_STORE *ss, int client_fd) {
    REQ_STATE *state = get_state(ss, client_fd);
    if(state == NULL) {
        fprintf(stderr, "Invalid state: not initialized\n");
        return -1;
    }

    if(state->status != WAITING_INPUT) {
        fprintf(stderr, "Unexpected input, ignoring...\n");
        return -1;
    }
    
    // Handle first time input event for clients
    // Subsequent inputs from client will be ignored
    if(state->type == STATE_CLIENT) {
        // Read requested file path from fd
        char path[110] = "assets/";
        char buff[100];
        int received;
        
        if((received = recv(client_fd, buff, 100, 0)) == 0) {
            return 0;
        }

        buff[received] = '\0';

        strncat(path, buff, received);

        printf("Opening: '%s'\n", path);

        int fd = open(path, O_RDONLY | O_NONBLOCK, 0);
        if(fd < 0) {
            perror("open error");
            return 0; 
        }

        // Initialize new state to track requested file for streaming
        REQ_STATE *asset_state = new_state(fd, STATE_ASSET); 
        asset_state->parent = state; 
        asset_state->status = WAITING_INPUT;
        set_state(ss, fd, asset_state);
        state->status = WAITING_READ;

        struct aiocb *aioctl = malloc(sizeof(struct aiocb));
        char *buf = (char *) malloc(SERVER_READ_BUF_SIZE);
        aioctl->aio_fildes = fd;
        aioctl->aio_buf = buf;
        aioctl->aio_nbytes = SERVER_READ_BUF_SIZE;
        if(aio_read(aioctl) < 0) {
            perror("aio_read: ");
            return -1;
        }

        asset_state->aioctl = aioctl;

        return fd;   
    }

    if(state->type == STATE_ASSET) {
        char *buf = (char *) state->aioctl->aio_buf;
        int nread = state->aioctl->aio_nbytes;

        int stat = aio_error(state->aioctl);
        if(stat == 0) {
            nread = aio_return(state->aioctl);
            if(nread == EOF) {
                printf("reached EOF, closing fd...\n");
                close(state->aioctl->aio_fildes); 
                return 0;
            }
            buf[nread] = '\0';
            printf("aio_read success: read %d bytes\n", nread);
            printf("Data read: %s\n", buf);

            int client_fd = state->parent->conn_fd;

            struct aiocb *writectl = malloc(sizeof(struct aiocb));
            writectl->aio_fildes = client_fd;
            writectl->aio_buf = buf;
            writectl->aio_nbytes = SERVER_READ_BUF_SIZE;
            if(aio_write(writectl) < 0) {
                perror("aio_write: ");
                return -1;
            }

            if(aio_read(state->aioctl) < 0) {
                perror("aio_read: ");
                return -1;
            }

            return 1;
        } else if(stat == EINPROGRESS) {
            printf("aio_read hasn't completed\n");
            return 1;
        } else if(stat == ECANCELED) {
            printf("aio_read has been canceled\n");
        } else if(stat > 0) {
            fprintf(stderr, "aio_read has failed: ");
            if(stat == EINVAL) {
                fprintf(stderr, "invalid aio control block");
            } else if(stat == ENOSYS) {
                fprintf(stderr, "aio_error is not implemented in this architecture");
            } else {
                fprintf(stderr, "unexpected failure");
            }
            printf("\n");

            return -1;
        } else {
            printf("unexpected aio_read status\n");
        }
       
        return 0;
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

void handle_new_connection(STATE_STORE *ss, int conn_fd) {
    REQ_STATE *state = new_state(conn_fd, STATE_CLIENT); 

    set_state(ss, conn_fd, state);
}
