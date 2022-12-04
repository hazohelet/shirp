#include "shirp.h"

#define READLINE_BUFSIZE 32

extern Token *cur;
extern bool lexical_error;
extern bool syntax_error;
extern bool eval_error;
extern bool side_effect;
extern Obj *builtin;
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

char *read_file(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    fprintf(stderr, "shirp: cannot open file '%s'\n", filename);
    exit(1);
  }
  char *buffer;
  size_t bufsize;
  FILE *out = open_memstream(&buffer, &bufsize);

  // copy the whole content
  for (;;) {
    char buf[4096];
    size_t read_bytes = fread(buf, 1, sizeof(buf), fp);
    if (read_bytes == 0)
      break;
    fwrite(buf, 1, read_bytes, out);
  }
  fclose(fp);

  fflush(out);
  fputc('\0', out);
  fclose(out);
  return buffer;
}

Obj *new_builtin(char *name) {
  Obj *obj = new_obj(BUILTIN_TY);
  obj->exclusive.str_val = name;
  return obj;
}

void register_builtin(char *name) {
  Obj *obj = new_builtin(name);
  frame_insert_obj(env, name, strlen(name), obj);
}

Obj *evaluate_string(char *str) {
  lexical_error = false;
  syntax_error = false;
  eval_error = false;
  side_effect = false;
  Token head = {};
  tokenize(str, &head);
  dump_tokens(head.next);
  if (lexical_error) {
    free(str);
    free_tokens(head.next);
    return NULL;
  }
  if (!head.next || lexical_error) {
    free(str);
    return NULL;
  }
  /* tokenization finished */
  cur = head.next;
  ASTNode *ast = program();
  if (syntax_error) {
    free(str);
    free_tokens(head.next);
    free_ast(ast);
    return NULL;
  }

  if (cur) {
    tok_error_at(cur, "fatal: still tokens left");
    return NULL;
  }
  debug_log("/* Parsing finished */\n");
  mark_tail_calls(ast, false);
  debug_log("/* Tail calls are marked */\n");
  Obj *res = eval_ast(ast);
  if (eval_error) {
    free(str);
    free_tokens(head.next);
    free_ast(ast);
    free_obj(res);
    return NULL;
  }
  dump_env(env);

  if (!side_effect) {
    free(str);
    free_tokens(head.next);
    free_ast(ast);
  }
  return res;
}

/* if load successful, return true. if fail: false */
bool load_file(char *filename) {
  char *buffer = read_file(filename);
  if (!buffer)
    return false;
  evaluate_string(buffer);
  return lexical_error || syntax_error || eval_error ? false : true;
}

void shirp_init() {
  env = push_new_frame(NULL);
  GC_init();
  register_builtin("+");
  register_builtin("-");
  register_builtin("*");
  register_builtin("/");
  register_builtin("=");
  register_builtin("<");
  register_builtin(">");
  register_builtin("<=");
  register_builtin(">=");
  register_builtin("div");
  register_builtin("remainder");
  register_builtin("car");
  register_builtin("cdr");
  register_builtin("null?");
  register_builtin("pair?");
  register_builtin("list?");
  register_builtin("symbol?");
  register_builtin("number?");
  register_builtin("list");
  register_builtin("cons");
  register_builtin("and");
  register_builtin("or");
  register_builtin("eq?");
  register_builtin("equal?");
  register_builtin("sqrt");
  register_builtin("load");
  register_builtin("even?");
  register_builtin("odd?");
  load_file("prelude.scm");
}

#ifndef TEST
int main(int argc, char **argv) {
  shirp_init();
  if (argc >= 2) {
    char *filename = argv[1];
    char *buffer = read_file(filename);
    if (buffer)
      evaluate_string(buffer);
    else
      printf("error\n");
  }
  do {
    fprintf(stderr, ">> ");

    size_t bufsize = READLINE_BUFSIZE;
    size_t pos = 0;
    char *line = (char *)shirp_calloc(bufsize, sizeof(char));
    int brackets_left = 0;
    do {
      if (brackets_left > 0)
        fprintf(stderr, ".. ");
      line = shirp_readline(line, &pos, &bufsize, &brackets_left);
    } while (brackets_left > 0);

    Obj *res = evaluate_string(line);
    if (res)
      println_obj(res);
    GC_collect();
  } while (1);

  return 0;
}
#endif