#include "shirp.h"

void *shirp_malloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "shirp: allocation error");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void dump_tokens(Token *tokens) {
  if (!tokens) {
    printf("NULL\n");
    return;
  }
  Token *tok = tokens;
  do {
    if (tok && tok->kind == TOKEN_INTEGER)
      fprintf(stderr, "`%.*s`[I'%ld]-> ", (int)tok->len, tok->loc,
              tok->val.int_val);
    else if (tok && tok->kind == TOKEN_FLOAT)
      fprintf(stderr, "`%.*s`[F'%lf]-> ", (int)tok->len, tok->loc,
              tok->val.float_val);
    else
      fprintf(stderr, "`%.*s` (%d)-> ", (int)tok->len, tok->loc, tok->kind);
    tok = tok->next;
  } while (tok);
  fprintf(stderr, "NULL\n");
}