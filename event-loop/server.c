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

    int client_fd;
    fd_set listen_fds, active_fds;

    struct sockaddr client_addr;
    int addrsize = sizeof client_addr; 
    struct timeval conn_timeout; 
    conn_timeout.tv_sec = 10; 
    conn_timeout.tv_usec = 0;

    int max_fd = client_fd = sock_fd + 1;

    while(1) {
        FD_ZERO(&listen_fds);
        FD_SET(sock_fd, &listen_fds);
        for(int i = sock_fd + 1; i < max_fd; i++) {
            if(!FD_ISSET(i, &active_fds)) {
                continue;
            }
            FD_SET(i, &listen_fds);
        }
        int rc = select(client_fd + 1, &listen_fds, NULL, NULL, &conn_timeout);

        conn_timeout.tv_sec = 10;

        if(FD_ISSET(sock_fd, &listen_fds)) {
            if((client_fd = accept(sock_fd, &client_addr, &addrsize)) < 0) {
                perror("accept error");
                return 1;
            }

            printf("Client connected\n");

            FD_SET(client_fd, &active_fds);
            handle_connected(client_fd);
        }

        max_fd = client_fd + 1;
        
        for(int i = sock_fd + 1; i < max_fd + 1; i++) {
            if(!FD_ISSET(i, &listen_fds)) {
                continue;
            }

            if(handle_received_from_client(i) == 0) {
                printf("Client disconnected\n");
                if(i == client_fd) {
                    printf("Most recent client disconnected\n");
                    client_fd -= 1;
                    continue;
                }
                FD_CLR(i, &active_fds);
            }
        }
    }

    return 0;
}

void handle_signal(int sig) {
    while(waitpid(-1, NULL, WNOHANG) > 0) {}
} 
