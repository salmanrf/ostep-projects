#include <stdio.h>
#include <stdlib.h>

int decompress_rle(FILE *in, FILE *out);

int main(int argc, char *argv[]) {
  if(argc < 2) {
    int s = decompress_rle(stdin, stdout);
    if(s != 0) {
      fprintf(stderr, "wunzip: failed to decompress\n");
      exit(1);
    }
  }

  for(int i = 1; i < argc; i++) {
    char *path = argv[i];
    FILE *in = fopen(path, "r");
    if(in == NULL) {
      fprintf(stderr, "wunzip: unable to open file %s\n", path);
      exit(1);
    }

    int s = decompress_rle(in, stdout);
    if(s != 0) {
      fprintf(stderr, "wunzip: failed to decompress %s\n", path);
      exit(1);
    }

    fclose(in);
  }

  return 0;
}

int decompress_rle(FILE *in, FILE *out) {
  int count = 0;
  char ch = '\0';

  int n_read = fread(&count, sizeof(int), 1, in);
  int ch_read = fread(&ch, sizeof(char), 1, in);

  if(n_read <= 0 || ch_read <= 0) {
    return 1;
  }
 
  while(n_read > 0 && ch_read > 0) {
    for(int i = 0; i < count; i++) {
      fprintf(out, "%c", ch);
    }
    
    n_read = fread(&count, sizeof(int), 1, in);
    ch_read = fread(&ch, sizeof(char), 1, in);
  }

  return 0;
}
