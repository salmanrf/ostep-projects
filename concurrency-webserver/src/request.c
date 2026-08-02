#include "request.h"

//
// Some of this code stolen from Bryant/O'Halloran
// Hopefully this is not a problem ... :)
//

#define MAXBUF (8192)

Request *init_request_metadata(int fd) {
    char *buf = NULL;
    int len = 0;
	int stat = readnextline(fd, &buf, &len);
	if (stat < 0) {
		return NULL;
	}

    char method[len - 3], uri[len - 3], version[len -3];

	sscanf(buf, "%s %s %s", method, uri, version);

    if(strcasecmp(method, "GET")) {
        return NULL;
    }

    Request *req = malloc(sizeof(Request));
    req->Method = (char *) malloc(strlen(method) + 1);
    req->Path = (char *) malloc(strlen(uri) + 1);
    req->Version = (char *) malloc(strlen(version) + 1);
    strcpy(req->Method, method);
    strcpy(req->Path, uri);
    strcpy(req->Version, version);

    return req;
}

int get_req_args(Request *req, int conn_fd) {
    req->req_filename = "";

	char *filename = NULL; 
	char *cgiargs = NULL;
	struct stat *sbuf = malloc(sizeof(struct  stat));
	request_parse_uri(req->Path, &filename, &cgiargs);

    if(strcmp(filename, "") == 0 && strcmp(cgiargs, "") == 0) {
        printf("Must provide either static path, or cgi parameters");
        return -1;
    }

    if(strcmp(cgiargs, "") != 0) {
        req->is_cgi_req = 1;
    } else {
        req->is_cgi_req = 0;
    }

	if (stat(filename, sbuf) < 0) {
		request_error(conn_fd, filename, "404", "Not found", "server could not find this file");
		return -1;
    }

	req->req_filename = filename;
	req->req_cgiargs = cgiargs;
    req->filestat = sbuf;

	return 0;
}

void request_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXBUF], body[MAXBUF];
    
    // Create the body of error message first (have to know its length for header)
    sprintf(body, ""
        "<!doctype html>\r\n"
        "<head>\r\n"
        "  <title>OSTEP WebServer Error</title>\r\n"
        "</head>\r\n"
        "<body>\r\n"
        "  <h2>%s: %s</h2>\r\n" 
        "  <p>%s: %s</p>\r\n"
        "</body>\r\n"
        "</html>\r\n", errnum, shortmsg, longmsg, cause);
    
    // Write out the header information for this response
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    write_or_die(fd, buf, strlen(buf));
    
    sprintf(buf, "Content-Type: text/html\r\n");
    write_or_die(fd, buf, strlen(buf));
    
    sprintf(buf, "Content-Length: %lu\r\n\r\n", strlen(body));
    write_or_die(fd, buf, strlen(buf));
    
    // Write out the body last
    write_or_die(fd, body, strlen(body));
}

//
// Reads and discards everything up to an empty text line
//
void request_read_headers(int fd) {
    char buf[MAXBUF];
    
    readline_or_die(fd, buf, MAXBUF);
    while (strcmp(buf, "\r\n")) {
		readline_or_die(fd, buf, MAXBUF);
    }
    return;
}

//
// Return 1 if static, 0 if dynamic content
// Calculates filename (and cgiargs, for dynamic) from uri
//
int request_parse_uri(char *uri, char **filename, char **cgiargs) {
    char *temp = NULL;
    
    // * Static
    if (!strstr(uri, "cgi")) { 
        *cgiargs = "";
        temp = (char *) malloc(strlen(uri) + 1);
		sprintf(temp, ".%s", uri);

		if (uri[strlen(uri)-1] == '/') {
            temp = "index.html";
		}

        *filename = temp;
		
		return 0;
    }
    
    // * Dynamic
    char *ptr = index(uri, '?');
    if (ptr) {
        temp = ptr + 1;
        *cgiargs = malloc(strlen(temp));
        strcpy(*cgiargs, temp);

        *ptr = '\0';
    } else {
        *cgiargs = "";
    }

    return 0;
}

//
// * Fills in the filetype given the filename
//
void request_get_filetype(char *filename, char **filetype) {
    char *temp = malloc(strlen(filename));
    
    if (strstr(filename, ".html")) {
        strcpy(temp, "text/html");
    } else if (strstr(filename, ".gif")) {
        strcpy(temp, "image/gif");
    } else if (strstr(filename, ".jpg")) {
        strcpy(temp, "image/jpeg");
    } else {
        strcpy(temp, "text/plain");
    }

    int nlen = strlen(temp);
    temp[nlen - 1] = '\0';
    temp = realloc(temp, nlen);
    *filetype = temp;
}

// TODO: Test & make work
int request_serve_dynamic(int fd, char *filename, char *cgiargs) {
    char buf[MAXBUF], *argv[] = { NULL };

    // The server does only a little bit of the header.  
    // The CGI script has to finish writing out the header.
    sprintf(
		buf, ""
		"HTTP/1.0 200 OK\r\n"
		"Server: OSTEP WebServer\r\n");

    write_or_die(fd, buf, strlen(buf));

    if (fork_or_die() == 0) {                        // child
	    setenv_or_die("QUERY_STRING", cgiargs, 1);   // args to cgi go here
	    dup2_or_die(fd, STDOUT_FILENO);              // make cgi writes go to socket (not screen)
	    extern char **environ;                       // defined by libc 
        execve_or_die(filename, argv, environ);
    } else {
        wait_or_die(NULL);
    }

    return 0;
}

int request_serve_static(Request *req) {
    int conn_fd = req->fd;
    char *req_filename = req->req_filename;
    int req_filesize = req->filestat->st_size;

    int srcfd;
    char *filetype;
    char buf[MAXBUF];
    char *srcp;

    request_get_filetype(req_filename, &filetype);
    if((srcfd = open(req_filename, O_RDONLY, 0)) < 0) {
        perror("Unable to open file:");
        return -1;
    }

    // * Rather than call read() to read the file into memory, 
    // * which would require that we allocate a buffer, we memory-map the file
    if((srcp = mmap(0, req_filesize, PROT_READ, MAP_PRIVATE, srcfd, 0)) < 0) {
        perror("Unable to load file for response:");
        return -1;
    }
    close(srcfd);

    // * Put together response
    sprintf(buf, ""
        "HTTP/1.0 200 OK\r\n"
        "Server: OSTEP WebServer\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: %s\r\n\r\n", 
        req_filesize, filetype);

    if(write(conn_fd, buf, strlen(buf)) < 0) {
        perror("Unable to write response headers:");
        return -1;
    }

    // * Writes out to the client socket the memory-mapped file 
    if(write(conn_fd, srcp, req_filesize) < 0) {
        perror("Unable to write response body:");
        return -1;
    }
    
    if(munmap(srcp, req_filesize) < 0) {
        perror("Unable to unmap:");
        return -1;
    }

    return 0;
}

// * Handle a request
int request_handle(Request *req) {
    int conn_fd = req->fd;
    char *req_filename = req->req_filename;
    char *req_cgiargs = req->req_cgiargs;
    int is_cgi = req->is_cgi_req;
    struct stat *file_meta = req->filestat;
    
    if (!is_cgi) {
        if (!(S_ISREG(file_meta->st_mode)) || !(S_IRUSR & file_meta->st_mode)) {
            request_error(conn_fd, req_filename, "403", "Forbidden", "server could not read this file");
            return -1;
        }

        return request_serve_static(req);
    } else {
        if (!(S_ISREG(file_meta->st_mode)) || !(S_IXUSR & file_meta->st_mode)) {
            request_error(conn_fd, req_filename, "403", "Forbidden", "server could not run this CGI program");
            return -1;
        }

        request_serve_dynamic(conn_fd, req_filename, req_cgiargs);
    }

    return 0;
}
