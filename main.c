#include "shirp.h"

#define READLINE_BUFSIZE 1024

extern Token *cur;
extern bool syntax_error;

char *shirp_readline() {
  int pos = 0;
  size_t bufsize = READLINE_BUFSIZE;
  char *buffer = (char *)shirp_malloc(bufsize * sizeof(char));
  char c;

  while (1) {
    c = (char)getchar();

    if (c == '\n') {
      buffer[pos] = '\0';
      return buffer;
    } else if (c == EOF) {
      printf("\n");
      exit(-1);
    } else {
      buffer[pos] = c;
    }
    pos++;

    if (pos > (int)bufsize) {
      bufsize += READLINE_BUFSIZE;
      buffer = (char *)realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "bshell: allocation error");
        exit(EXIT_FAILURE);
      }
    }
  }
}

int main() {
  do {
    fprintf(stderr, ">> ");

    char *line = shirp_readline();

    tokenize(line);
    Token *head = cur;
    dump_tokens(head);

    free(line);
  } while (1);

  return 0;
}