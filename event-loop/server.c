#include "common.h"
#include "server.h"

int start_server();

void handle_signal(int sig);

int main() {
    int stat;
    if((stat = start_server()) != 0) {
        fprintf(stderr, "server terminated with error\n"); 
        return stat;
    }

    return 0;
}

int start_server() {
    int sock_fd;
    struct addrinfo hints, *servinfo, *node;
    int stat;
    int ok = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((stat = getaddrinfo(NULL, SERVICE, &hints, &servinfo)) < 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(stat));
        return stat;
    }

    for(node = servinfo; node != NULL; node = node->ai_next) {
        if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket error");
            continue;
        }
        
        if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(int)) < 0) {
            perror("setsockopt");
            continue;
        }

        if(bind(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
            perror("bind error");
            continue; 
        }

        break;
    }
    freeaddrinfo(servinfo);
    if(node == NULL) {
        fprintf(stderr, "error: no address to bind\n");
        return 1;
    }
   
    if(listen(sock_fd, SERVER_BACKLOG) < 0) {
        perror("setsockopt");
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("sigaction error");
        return 1;
    }

    printf("Server is ready to accept connections, listening on :%s\n", SERVICE);

    int curr_numfds = 1 + (2 * MAX_CONNECTION);
    short events = POLLIN;
    struct pollfd *pollfds = build_pollfds(curr_numfds, sock_fd, events);

    int client_fd, current_n_conns;
    current_n_conns = 0;

    struct sockaddr client_addr;
    socklen_t addrsize = sizeof client_addr; 

    pollfds[0].fd = sock_fd;
    pollfds[0].events = events;

    STATE_STORE *state_store = init_state_store(3 * MAX_CONNECTION);

    while(1) {
        // TODO: poll returns fds that can be read without blocking, that means aio_read will count even if the buffer is not full yet!
        int stat = poll(pollfds, curr_numfds, POLL_TIMEOUT_MS);
        if(stat < 0) {
            perror("poll: ");
            exit(1);
        }

        if(stat == 0) {
            printf("Timeout reached without events\n");
            continue;
        }

        printf("Ready: %d\n", stat);

        int processed = 0;
        for(int i = 0; i < curr_numfds; i++) {
            if(processed >= stat) {
                break;
            }

            struct pollfd *pfd = &pollfds[i]; 
            if(pfd->revents & POLLHUP) {
                printf("FD not opened!\n");
                pfd->fd = -1 * abs(pfd->fd);
                current_n_conns -= 1;
                processed += 1;
            }

            // Process read-ready FDs
            if(pfd->revents & POLLIN) {
                // Event comes from server, likely a connection request
                if(pfd->fd == sock_fd) {
                    if(current_n_conns + 1 > MAX_CONNECTION) {
                        pfd->fd = -1 * abs(pfd->fd);
                        processed += 1;
                        continue;
                    }
                    if((client_fd = accept(sock_fd, &client_addr, &addrsize)) < 0) {
                        perror("accept: ");
                        processed += 1;
                        break;
                    }

                    printf("New connection has been opened\n"); 

                    // Index into the pollfds to get the client handler
                    // Client fds start at immediately after server's fd (sock_fd)
                    struct pollfd *client = &pollfds[client_fd - sock_fd];
                    // Set the fd in pollfd to positive val to start listening to the new client's fd
                    client->fd = abs(client->fd);
                    handle_new_connection(state_store, client->fd);
                    current_n_conns += 1;
                    processed += 1;
                    continue;
                }
                
                printf("Client ready to read!\n");
                int res = handle_input(state_store, pfd->fd);
                if(res <= 0) {
                    if(res == 0) {
                        printf("Client has been served successfully\n");
                    } else {
                        fprintf(stderr, "Client input processed with error result\n");
                    }
                    close(pfd->fd);
                    pfd->fd = -1 * abs(pfd->fd);
                    current_n_conns -= 1;
                    (&pollfds[0])->fd = abs((&pollfds[0])->fd);
                } else {
                    // res > 0 but not equal to the original fd, this means a new fd has been opened (for reading disk file)
                    if(res != pfd->fd) {
                        // Start listening for the new fd
                        struct pollfd *npfd = &pollfds[res - sock_fd]; 
                        npfd->fd = abs(npfd->fd);
                    }
                    printf("Client input has been processed, waiting for next step\n");
                }

                processed += 1;
            }
        }
    }

    return 0;
}

void handle_signal(int sig) {
    printf("Received signal %d", sig);
    while(waitpid(-1, NULL, WNOHANG) > 0) {}
} 
