#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int getnextline(char **lineptr, int *size, FILE *fp);

int main(int argc, char *argv[]) {
  if(argc < 2) {
    fprintf(stdout, "wgrep: searchterm [file ...]\n");
    exit(1);
  }

  char *keyword = argv[1];
  char *filepath = argv[2];

  FILE *file = NULL;

  if(argc < 3) {
    file = stdin;
  } else {
    file = fopen(filepath, "r");
 }
 
  if(file == NULL) {
    fprintf(stdout, "wgrep: cannot open file\n");
    exit(1);
  }
  
  char *line;
  int linesize = 0;

  while(getnextline(&line, &linesize, file) == 0) {
    if(strstr(line, keyword) != NULL) {
      printf("%s", line);
    }
    free(line);
  }

  return 0;
}

int getnextline(char **lineptr, int *size, FILE *fp) {
  int currsize = 100;
  char *temp = (char *) malloc(currsize);

  int n = 0;
  int ch = '\0';

  while((ch = fgetc(fp)) != EOF) { 
    if(n >= currsize) {
        temp = (char *) realloc(temp, currsize + 100);
        currsize += 100;
     }

     temp[n] = (char) ch;
     n += 1;

     if(ch == '\n') {
       temp = realloc(temp, n);
       temp[n] = '\0';

       *lineptr = temp;
       *size = n;

       return 0;
     }
  }

  free(temp);

  return 1;
}
