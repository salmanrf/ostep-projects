#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
  struct Node *next;
  struct Node *prev;
  char *line;
  size_t linesize_t;
} Node;

int main(int argc, char *argv[]) {
  int output_fd = 1;
  int input_fd = 0;
  char *output_file_path = NULL;
  char *input_file_path = NULL;

  if(argc > 3) {
    fprintf(stderr, "usage: reverse <input> <output>\n");
    exit(1);
  }

  if (argc > 1) {
    input_file_path = argv[1];
    input_fd = open(input_file_path, O_RDONLY);
    if (input_fd < 0) {
      fprintf(stderr, "reverse: cannot open file '%s'\n", input_file_path);
      exit(1);
    }
  }

  if (argc > 2) {
    output_file_path = argv[2];

    if(strcmp(input_file_path, output_file_path) == 0) {
      fprintf(stderr, "reverse: input and output file must differ\n");
      exit(1);
    }

    output_fd = open(output_file_path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    if (output_fd < 0) {
      fprintf(stderr, "reverse: cannot open file '%s'\n", output_file_path);
      exit(1);
    }
  }

  struct stat input_file_stat;
  struct stat output_file_stat;

  int status = fstat(input_fd, &input_file_stat);

  if(status < 0) {
    exit(1);
  }

  status = fstat(output_fd, &output_file_stat);

  if(status < 0) {
    exit(1);
  }

  if(input_file_stat.st_ino == output_file_stat.st_ino) {
    fprintf(stderr, "reverse: input and output file must differ\n");
    exit(1);
  }

  FILE *input_file = fdopen(input_fd, "r");
  FILE *output_file = fdopen(output_fd, "wr");

  char *line = NULL;
  size_t linesize;

  Node *head = NULL;
  Node *node = head;

  while(getline(&line, &linesize, input_file) != EOF) {
    Node *new_node = (Node *) malloc(sizeof(Node));
    char *newline = (char *) malloc(linesize);
    new_node->line = strcpy(newline, line);
    new_node->linesize_t = linesize;
    new_node->prev = node;

    if(node == NULL) {
      head = new_node;
    } else {
      node->next = new_node;
    }
    
    node = new_node;
  }

  while(node != NULL) {
    fprintf(output_file, "%s", node->line);
    node = node->prev;
  }

  fclose(input_file);
  fclose(output_file);

  return 0;
}

