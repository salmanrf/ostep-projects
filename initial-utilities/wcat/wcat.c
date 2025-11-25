#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

void cat_file(char *file_path);

int main(int argc, char *argv[]) {
  if(argc < 2) {
    return 0;
  }

  for(int i = 1; i < argc; i++) {
    cat_file(argv[i]);
  }

  return 0;
}

void cat_file(char *file_path) {
  int fd = 0;
  
  if(file_path != NULL) {
    fd = open(file_path, O_RDONLY);
  }

  if(fd < 0) {
    fprintf(stdout, "wcat: cannot open file\n");
    exit(1);
  }

  char *red = (char *) malloc(100); 
  int n = 0;
  while((n = read(fd, red, 100)) > 0) {
    if(n < 100) {
	red[n] = '\0';
    }
    printf("%s", red);
  }

  close(fd);
}
