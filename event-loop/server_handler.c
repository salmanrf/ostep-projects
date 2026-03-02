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
