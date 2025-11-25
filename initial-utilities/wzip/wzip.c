#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int compress_rle(int n_file, FILE **fp, FILE *out);
int getc_from_files(int n_file, FILE **fp);

int main(int argc, char *argv[]) {
  if(argc < 2) {
    fprintf(stdout, "wzip: file1 [file2 ...]\n");
    exit(1);
  }

  int n_file = argc - 1;
  FILE **files = (FILE **) malloc(sizeof(FILE *) * n_file);

  for(int i = 0; i < n_file; i++) {
    char *path = argv[i + 1];
    FILE *fp = fopen(path, "r");
    if(fp == NULL) {
      fprintf(stderr, "wzip: unable to open file %s\n", path);
      exit(1);
    }
    
    files[i] = fp;
  }

  int s = compress_rle(n_file, files, stdout);

  if(s != 0) {
    fprintf(stderr, "wzip: error compressing\n");
    exit(1);
  }

  for(int i = 0; i < n_file; i++) {
    fclose(files[i]);
  }

  free(files);

  return 0;
}

int getc_from_files(int n_file, FILE **fp) {
  if(n_file == 0) {
    return EOF;
  }

  int ch = EOF;

  for(int i = 0; i < n_file; i++) {
     ch = fgetc(fp[i]);
     if(ch != EOF) {
       return ch;     
     }
  }

  return EOF;
}

int compress_rle(int n_file, FILE **fp, FILE *out) {
  int ch = getc_from_files(n_file, fp);
  char last_ch = ch;
  int n_ch = 1;

  if(ch == EOF) {
    return 1;
  }

  while((ch = getc_from_files(n_file, fp)) != EOF) {
    // Reset counter on newline
    if(last_ch == '\n') {
	last_ch = ch;
        n_ch = 1; 
	continue;
    }

    // Write upon end of consecutive characters, or newline
    if(ch != last_ch || ch == '\n') {
      int n_written = fwrite(&n_ch, sizeof(int), 1, out);
      if(n_written == 0) {
        fprintf(stderr, "failed to write, terminating\n");
	return 1;
      }

      n_written = fwrite(&last_ch, sizeof(char), 1, out);
      if(n_written == 0) {
        fprintf(stderr, "failed to write, terminating\n");
	return 1;
      }

      if (ch == '\n') {
        fwrite(&ch, sizeof(char), 1, out);
      }

      n_ch = 1;
    } else {
      n_ch += 1;
    }

    last_ch = ch;
  }

  int n_written = 0;

  if(last_ch != '\n') {
     n_written = fwrite(&n_ch, sizeof(int), 1, out);
    if(n_written == 0) {
      fprintf(stderr, "failed to write, terminating\n");
      return 1;
    }
  }

  n_written = fwrite(&last_ch, sizeof(char), 1, out);
  if(n_written == 0) {
    fprintf(stderr, "failed to write, terminating\n");
    return 1;
  }
   
  return 0; 
}
