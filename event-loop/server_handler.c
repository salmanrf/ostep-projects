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

int handle_received_from_client(int client_fd) {
    char path[110] = "assets/";
    char buff[100];
    int received;

    if((received = recv(client_fd, buff, 100, 0)) > 0) {
        buff[received] = '\0';

        strncat(path, buff, received);

        int fd = open(path, O_RDONLY, 0);
        if(fd < 0) {
            perror("open error");
            return -1; 
        }

        char rdbuff[100];
        int nread;
        int nwrite;
        while((nread = read(fd, rdbuff, 100)) > 0) {
            rdbuff[nread] = '\0';
            printf("FORWARD: %s\n", rdbuff);
            if((nwrite = send(client_fd, rdbuff, nread, 0)) < nread) {
               perror("incomplete data sent"); 
            }
        }

        close(fd);
        return 1;
    }

    return 0;
}
