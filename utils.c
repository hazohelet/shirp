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
void verror_at(char *loc, char *fmt, va_list ap) {
  int offset = PROMPT_OFFSET;
  for (char *p = loc; *p && *p != '\n'; p--)
    offset++;
  offset--;
  fprintf(stderr, "%*s^ ", offset, "");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
}

void vdebug_log(char *fmt, va_list ap) {
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
}

void debug_log(char *fmt, ...) {
#ifdef DEBUG
  va_list ap;
  va_start(ap, fmt);
  vdebug_log(fmt, ap);
#endif
  fmt = fmt; // dummy for avoiding unused-variable warning
}

/*
  Tokenizer Utility Functions
*/

bool match_str(char *str, char *keyword, size_t len) {
  return strncmp(str, keyword, len) == 0;
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
    /*
    if (tok && tok->kind == TOKEN_NUMBER) {
      Obj *obj = tok->obj;
      if (obj->typ == INT_TY)
        fprintf(stderr, "`%.*s`[I'%ld]-> ", (int)tok->len, tok->loc,
                obj->num_val.int_val);
      else
        fprintf(stderr, "`%.*s`[F'%lf]-> ", (int)tok->len, tok->loc,
                obj->num_val.float_val);
    } else
    */
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
  fprintf(stderr, "---HashTable---\n");
  for (size_t i = 0; i < ht->capacity; i++) {
    Entry *entry = ht->buckets[i];
    if (entry && entry != TOMBSTONE) {
      fprintf(stderr, "`%.*s` registered\n", (int)entry->keylen, entry->key);
    }
  }
  fprintf(stderr, "---HashTable--END---\n");
}
