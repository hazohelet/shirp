#include "shirp.h"

#define READLINE_BUFSIZE 1024

extern Token *cur;
extern bool lexical_error;
extern int brackets_left;

char *shirp_readline(char *buffer, size_t *pos, size_t *bufsize) {
  char c;

  while (1) {
    c = (char)getchar();

    if (c == '\n') {
      buffer[*pos] = '\0';
      return buffer;
    } else if (c == EOF) {
      printf("\n");
      exit(-1);
    } else {
      buffer[*pos] = c;
    }
    (*pos)++;

    if (*pos > *bufsize) {
      *bufsize += READLINE_BUFSIZE;
      buffer = (char *)realloc(buffer, *bufsize);
      if (!buffer) {
        fprintf(stderr, "shirp: allocation error");
        exit(EXIT_FAILURE);
      }
    }
  }
}

int main() {
  do {
    fprintf(stderr, ">> ");

    size_t bufsize = READLINE_BUFSIZE;
    size_t pos = 0;
    char *line = (char *)shirp_malloc(bufsize * sizeof(char));
    line = shirp_readline(line, &pos, &bufsize);

    lexical_error = false;
    Token head = {};
    Token *tail = tokenize(line, &head);
    if (lexical_error) {
      free(line);
      continue;
    }
    while (brackets_left > 0) {
      fprintf(stderr, "... ");
      size_t tmp_pos = pos;
      line = shirp_readline(line, &pos, &bufsize);
      tail = tokenize(line + tmp_pos, tail);
    }
    dump_tokens(head.next);

    free(line);
  } while (1);

  return 0;
}