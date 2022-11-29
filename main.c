#include "shirp.h"

#define READLINE_BUFSIZE 32

extern Token *cur;
extern bool lexical_error;
extern bool syntax_error;
extern bool eval_error;
extern int brackets_left;
Frame *env;

char *shirp_readline(char *buffer, size_t *pos, size_t *bufsize) {
  char c;

  while (1) {
    int raw_c = getchar();
    c = (char)raw_c;

    if (c == '\n') {
      buffer[*pos] = '\0';
      return buffer;
    } else if (raw_c == EOF) {
      printf("\n");
      exit(-1);
    } else {
      buffer[(*pos)++] = c;
    }

    if (*pos >= *bufsize) {
      *bufsize *= 2;
      buffer = (char *)shirp_realloc(buffer, *bufsize);
      if (!buffer) {
        fprintf(stderr, "shirp: allocation error");
        exit(EXIT_FAILURE);
      }
    }
  }
}

void shirp_init() { env = push_new_frame(NULL); }

int main() {
  shirp_init();
  do {
    fprintf(stderr, ">> ");

    size_t bufsize = READLINE_BUFSIZE;
    size_t pos = 0;
    char *line = (char *)shirp_malloc(bufsize * sizeof(char));
    line = shirp_readline(line, &pos, &bufsize);

    brackets_left = 0;
    lexical_error = false;
    syntax_error = false;
    eval_error = false;
    Token head = {};
    Token *tail = tokenize(line, &head);
    if (lexical_error) {
      free(line);
      continue;
    }
    while (brackets_left > 0 && !lexical_error) {
      fprintf(stderr, ".. ");
      size_t tmp_pos = pos;
      line = shirp_readline(line, &pos, &bufsize);
      tail = tokenize(line + tmp_pos, tail);
    }
    if (!head.next || lexical_error) {
      free(line);
      continue;
    }
    /* tokenization finished */
    cur = head.next;
    dump_tokens(cur);
    ASTNode *ast = program();
    if (syntax_error) {
      free(line);
      continue;
    }
    if (cur) {
      tok_error_at(cur, "fatal: still tokens left");
      continue;
    }
    debug_log("/* Parsing finished */\n");
    Obj *res = eval_ast(ast);
    // dump_hashtable(env->table);
    if (eval_error) {
      continue;
    }
    dump_env(env);
    if (res)
      println_obj(res);
    else
      debug_log("nil");

  } while (1);

  return 0;
}