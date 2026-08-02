#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "io_helper.h"

typedef struct Request {
	int fd;
	char *Method;
	char *Path;
	char *Version;
	char *req_filename;
	char *req_cgiargs;
	int is_cgi_req;
	struct stat *filestat;
} Request;

Request *init_request_metadata(int fd);
int request_handle(Request *req);

void request_read_headers(int fd);
int request_parse_uri(char *uri, char **filename, char **cgiargs);
void request_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int get_req_args(Request *req, int conn_fd);

#endif
