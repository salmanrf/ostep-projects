#include "common.h"
#define ACCEPT_BUF_SIZE

int main(void) {
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

    char buff[100];
    while(1) {
       int received = recv(sock_fd, buff, 100, 0);
       if(received > 0 && received < 100) {
           buff[received] = '\0';
           printf("RECEIVED: '%s' from server\n", buff);
       }
    }
    
    close(sock_fd);

    return 0;
}
