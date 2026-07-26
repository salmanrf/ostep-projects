#include "common.h"
#include "server.h"

int start_server(int server_fd);
int create_server();

void handle_signal(int sig);

int main() {
    int stat, server_fd;
    if((server_fd = create_server()) < 0) {
        fprintf(stderr, "failed to create server\n");
        return -1 * server_fd;
    }

    if((stat = start_server(server_fd)) != 0) {
        fprintf(stderr, "server terminated with error\n"); 
        return stat;
    }

    return 0;
}

int start_server(int server_fd) {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("sigaction error");
        return 1;
    }

    int curr_numfds = 1 + (2 * MAX_CONNECTION);
    short events = POLLIN;
    struct pollfd *pollfds = build_pollfds(curr_numfds, server_fd, events);

    pollfds[0].fd = server_fd;
    pollfds[0].events = events;

    STATE_STORE *state_store = init_state_store(1 + (2 * MAX_CONNECTION));

    if(listen(server_fd, SERVER_BACKLOG) < 0) {
        perror("setsockopt");
        return 1;
    }
    printf("Server is ready to accept connections, listening on :%s\n", SERVICE);

    while(1) {
        int stat = poll(pollfds, curr_numfds, POLL_TIMEOUT_MS);
        if(stat < 0) {
            perror("poll: ");
            exit(1);
        }

        if(stat == 0) {
            printf("Timeout reached without events\n");
            continue;
        }

        int processed = 0;
        for(int i = 0; i < curr_numfds; i++) {
            if(processed >= stat) {
                break;
            }

            struct pollfd *pfd = &pollfds[i]; 
            
            EEvents event = get_event(state_store, server_fd, pfd);

            if(event == EMPTY) {
                continue;
            }
            processed += 1;

            switch (event) {
            case NEW_CONNECTION: {
                int new_fd = handle_new_connection(state_store, pollfds, pfd);
                if(new_fd < 0) {
                    close(pfd->fd);
                    pfd->fd = -1;
                } else {
                    int fdindex = new_fd - server_fd;
                    pollfds[fdindex].fd = abs(pollfds[fdindex].fd);
                }
                break;
            }
            case DISCONNECTED:
                handle_disconnected(state_store, pollfds, pfd);
                break;
            case CLIENT_IN_READY: {
                int new_fd = handle_client_input(state_store, pollfds, pfd);
                if(new_fd < 0) {
                    close(pfd->fd);
                    pfd->fd = -1;
                } else {
                    int fdindex = new_fd - server_fd;
                    pollfds[fdindex].fd = abs(pollfds[fdindex].fd);
                }
                break;
            }
            case ASSET_IN_READY: {
                int stat = handle_asset_input(state_store, pollfds, pfd);
                if(stat < 0) {
                    REQ_STATE *state = get_state(state_store, pfd->fd);
                    int parent_i = state->parent->conn_fd - server_fd;
                    pollfds[parent_i].fd = - abs(pollfds[parent_i].fd);
                    pfd->fd = -1;
                    close(state->parent->conn_fd);
                }
                break;
            }
            default:
                break;
            }
        }
    }

    return 0;
}

int create_server() {
    int server_fd;
    struct addrinfo hints, *servinfo, *node;
    int stat;
    int ok;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((stat = getaddrinfo(NULL, SERVICE, &hints, &servinfo)) < 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(stat));
        return stat;
    }

    for(node = servinfo; node != NULL; node = node->ai_next) {
        if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket error");
            continue;
        }
        
        if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(int)) < 0) {
            perror("setsockopt");
            continue;
        }

        if(bind(server_fd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
            perror("bind error");
            continue; 
        }

        break;
    }
    freeaddrinfo(servinfo);
    if(node == NULL) {
        fprintf(stderr, "error: no address to bind\n");
        return -1;
    }

    return server_fd;
}

void handle_signal(int sig) {
    printf("Received signal %d", sig);
    while(waitpid(-1, NULL, WNOHANG) > 0) {}
} 
