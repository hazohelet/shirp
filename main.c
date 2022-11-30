#include "shirp.h"

#define READLINE_BUFSIZE 32

extern Token *cur;
extern bool lexical_error;
extern bool syntax_error;
extern bool eval_error;
Frame *env;

char *shirp_readline(char *buffer, size_t *pos, size_t *bufsize,
                     int *brackets_left) {
  char c;
  bool commenting = false;
  bool in_string = false;
  bool in_vbars = false;

  while (1) {
    int raw_c = getchar();
    c = (char)raw_c;

    if (*pos >= *bufsize - 1) {
      *bufsize *= 2;
      buffer = (char *)shirp_realloc(buffer, *bufsize);
      if (!buffer) {
        fprintf(stderr, "shirp: allocation error");
        exit(EXIT_FAILURE);
      }
    }

    if (c == '\n') {
      buffer[(*pos)++] = '\n';
      buffer[*pos] = '\0';
      return buffer;
    } else if (raw_c == EOF) {
      printf("\n");
      exit(-1);
    } else {
      if (!commenting && !in_string && !in_vbars) {
        if (c == '(')
          (*brackets_left)++;
        else if (c == ')')
          (*brackets_left)--;
      }
      if (c == ';')
        commenting = true;
      else if (c == '"')
        in_string = !in_string;
      else if (c == '|')
        in_vbars = !in_vbars;
      buffer[(*pos)++] = c;
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
    int brackets_left = 0;
    do {
      if (brackets_left > 0)
        fprintf(stderr, ".. ");
      line = shirp_readline(line, &pos, &bufsize, &brackets_left);
    } while (brackets_left > 0);

    lexical_error = false;
    syntax_error = false;
    eval_error = false;
    Token head = {};
    tokenize(line, &head);
    dump_tokens(head.next);
    if (lexical_error) {
      free(line);
      continue;
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
    mark_tail_calls(ast, false);
    debug_log("/* Tail calls are marked */\n");
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