#include "common.h"
#define ACCEPT_BUF_SIZE

int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "usage: client <requested_file_path>\n");
        exit(1);
    }

    int sock_fd;
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int stat;
    if((stat = getaddrinfo(NULL, SERVICE, &hints, &servinfo)) < 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(stat));
        exit(1);
    }

    
    struct addrinfo *node;
    for(node = servinfo; node != NULL; node = node->ai_next) {
        if((sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, 0)) < 0) {
            perror("client: socket");
            continue;
        }
    
        if(connect(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
            perror("client: connect");
            continue;
        }
        
        break;
    }

    if(node == NULL) {
        fprintf(stderr, "client: unable to connect\n");
        exit(1);
    }
    
    printf("Connected to the server\n");

    sleep(3);

    char *path = argv[1];
    size_t path_len = strlen(argv[1]);
    if(send(sock_fd, path, path_len, 0) < path_len) {
        fprintf(stderr, "Failed to request open: Incomplete write\n");
    }

    char buff[100];
    int received;
    while((received = recv(sock_fd, buff, 100, 0)) > 0) {
        if(received > 0 && received < 100) {
           buff[received] = '\0';
           printf("RECEIVED: '%s' from server\n", buff);
        }
    }
    printf("server closed the connection\n");
    
    close(sock_fd);

    return 0;
}
