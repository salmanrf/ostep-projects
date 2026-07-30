#include <stdio.h>
#include "request.h"
#include "io_helper.h"
#include "queue.h"

char default_root[] = ".";

pthread_cond_t requests_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t requests_empty = PTHREAD_COND_INITIALIZER;
pthread_mutex_t producer_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t consumer_m = PTHREAD_MUTEX_INITIALIZER;

int done = 0;

void *worker(void *args) {
	queue_t *requests = (queue_t *) args;

	while(1) {
		pthread_mutex_lock(&consumer_m);
		while(queue_is_empty(requests)) {
			pthread_cond_wait(&requests_empty, &consumer_m);
		}

		int conn_fd = *((int *) queue_dequeue(requests));
		request_handle(conn_fd);
		close_or_die(conn_fd);
		
		pthread_cond_signal(&requests_full);
		pthread_mutex_unlock(&consumer_m);
	}
}

int handle_requests(int server_fd, int num_threads) {
	queue_t *requests = queue_create();
	
	pthread_t *threads = (pthread_t *) malloc(sizeof num_threads);
	for(int i = 0; i < num_threads; i++) {
		pthread_create(&threads[0], NULL, worker, requests);
	}
	
	while (1) {
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		int conn_fd = accept_or_die(server_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);

		pthread_mutex_lock(&producer_m);
		while(queue_size(requests) >= num_threads) {
			pthread_cond_wait(&requests_full, &producer_m);
		}

		queue_enqueue(requests, &conn_fd);
		pthread_cond_signal(&requests_empty);
		pthread_mutex_unlock(&producer_m);
	}
	
	return 0;
}

//
// ./wserver [-d <basedir>] [-p <portnum>] 
// 
int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
	int threads = 5;
    
    while ((c = getopt(argc, argv, "d:p:t:")) != -1)
	switch (c) {
		case 'd':
			root_dir = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			threads = atoi(optarg);
			break;
		default:
			fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-t num threads]\n");
			exit(1);
	}

	printf("Arguments: basedir: '%s', port: %d,  num threads: %d\n", root_dir, port, threads);
	
    // * Run out of this directory
    chdir_or_die(root_dir);

    // * Now, get to work
    int listen_fd = open_listen_fd_or_die(port);
    handle_requests(listen_fd, threads);

    return 0;
}
