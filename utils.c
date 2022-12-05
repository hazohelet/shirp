#include "shirp.h"

// offset given by `>> ` repl prompt
#define PROMPT_OFFSET 3

void *shirp_malloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "shirp: allocation error");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *shirp_calloc(size_t n, size_t size) {
  void *ptr = calloc(n, size);
  if (!ptr) {
    fprintf(stderr, "shirp: allocation error");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *shirp_realloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  if (!new_ptr) {
    fprintf(stderr, "shirp: allocation error");
    exit(EXIT_FAILURE);
  }
  return new_ptr;
}

// reports error with its position
void verror_at(char *loc, size_t len, char *fmt, va_list ap) {
  char *head = loc;
  char *tail = loc;

  while (*(head - 1) && *(head - 1) != '\n')
    head--;
  while (*tail && *(tail + 1) && *(tail + 1) != '\n')
    tail++;
  int line_len = (int)(tail - head + 1);
  int indent = (int)(loc - head);

  fprintf(stderr, "\x1b[1m\x1b[31merror\x1b[0m: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  fprintf(stderr, "   %.*s\n", line_len, head);
  fprintf(stderr, "   %*s\x1b[1m\x1b[31m^\x1b[0m", indent, "");
  for (size_t i = 0; i < len - 1; i++)
    fprintf(stderr, "\x1b[1m\x1b[31m~\x1b[0m");
  fprintf(stderr, "\n");
}

const char *type_name(ObjType typ) {
  switch (typ) {
  case UNDEF_TY:
    return "undefined";
  case BOOL_TY:
    return "boolean";
  case INT_TY:
    return "integer";
  case FLOAT_TY:
    return "float";
  case CHAR_TY:
    return "character";
  case STRING_TY:
    return "string";
  case SYMBOL_TY:
    return "symbol";
  case LAMBDA_TY:
  case BUILTIN_TY:
    return "closure";
  case CONS_TY:
    return "pair";
  }
  return NULL;
}

void debug_printf(char *fmt, ...) {
#ifndef DEBUG
  return;
#endif
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

void debug_log(char *fmt, ...) {
#ifndef DEBUG
  return;
#endif
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

/*
  Tokenizer Utility Functions
*/

bool match_str(char *str, char *keyword, size_t len) {
  return memcmp(str, keyword, len) == 0;
}

bool match_tok(Token *tok, char *keyword) {
  return tok->len == strlen(keyword) && match_str(tok->loc, keyword, tok->len);
}

bool match_anyof_tok(Token *tok, char *keywords[]) {
  for (int i = 0; keywords[i]; i++)
    if (match_tok(tok, keywords[i]))
      return true;
  return false;
}

void dump_tokens(Token *tokens) {
#ifndef DEBUG
  return;
#endif
  if (!tokens) {
    printf("NULL\n");
    return;
  }
  Token *tok = tokens;
  do {
    if (tok && tok->kind == TOKEN_IMMEDIATE) {
      if (tok->typ == INT_TY)
        fprintf(stderr, "`%.*s`[I'%" PRId64 "]-> ", (int)tok->len, tok->loc,
                tok->val.int_val);
      else if (tok->typ == FLOAT_TY)
        fprintf(stderr, "`%.*s`[F'%lf]-> ", (int)tok->len, tok->loc,
                tok->val.float_val);
      else if (tok->typ == BOOL_TY)
        fprintf(stderr, "`%.*s`[V'%s]-> ", (int)tok->len, tok->loc,
                tok->val.bool_val ? "#t" : "#f");
    } else
      fprintf(stderr, "`%.*s` (%d)-> ", (int)tok->len, tok->loc, tok->kind);
    tok = tok->next;
  } while (tok);
  fprintf(stderr, "NULL\n");
}

#define TOMBSTONE ((void *)-1)

void dump_hashtable(HashTable *ht) {
#ifndef DEBUG
  return;
#endif
  for (size_t i = 0; i < ht->capacity; i++) {
    Entry *entry = ht->buckets[i];
    if (entry && entry != TOMBSTONE &&
        (*(Obj **)entry->val)->typ != BUILTIN_TY) {
      fprintf(stderr, "`%.*s`: ", (int)entry->keylen, entry->key);
      println_obj(*(Obj **)entry->val);
    }
  }
  fprintf(stderr, "-----------------\n");
}

void dump_env(Frame *env) {
#ifndef DEBUG
  return;
#endif
  fprintf(stderr, "---Environment---\n");
  while (env) {
    dump_hashtable(env->table);
    env = env->outer;
  }
}

void free_tokens(Token *tok) {
  if (!tok)
    return;
  free_tokens(tok->next);
  free(tok);
}

void free_ast(ASTNode *ast) {
  if (!ast)
    return;
  free_ast(ast->caller);
  free_ast(ast->args);
  free_ast(ast->listarg);
  free_ast(ast->next);
  free(ast);
}

void free_obj(Obj *obj) {
  if (!obj)
    return;
  if (obj->typ == LAMBDA_TY)
    pop_frame(obj->exclusive.saved_env);
  else if (obj->typ == STRING_TY || obj->typ == SYMBOL_TY)
    free(obj->exclusive.str_val);
  free(obj);
}
