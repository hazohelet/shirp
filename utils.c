#include "shirp.h"

void *shirp_malloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "shirp: allocation error");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

/*
  Tokenizer Utility Functions
*/

bool match(char *str, char *keyword, size_t len) {
  return strncmp(str, keyword, len) == 0;
}

void dump_tokens(Token *tokens) {
  if (!tokens) {
    printf("NULL\n");
    return;
  }
  Token *tok = tokens;
  do {
    if (tok && tok->kind == TOKEN_NUMBER) {
      Obj *obj = tok->obj;
      if (obj->typ == INT_TY)
        fprintf(stderr, "`%.*s`[I'%ld]-> ", (int)tok->len, tok->loc,
                obj->num_val.int_val);
      else
        fprintf(stderr, "`%.*s`[F'%lf]-> ", (int)tok->len, tok->loc,
                obj->num_val.float_val);
    } else
      fprintf(stderr, "`%.*s` (%d)-> ", (int)tok->len, tok->loc, tok->kind);
    tok = tok->next;
  } while (tok);
  fprintf(stderr, "NULL\n");
}
