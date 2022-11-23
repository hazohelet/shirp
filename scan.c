#include "shirp.h"

Token *cur;

static Token *new_token(TokenKind kind, char *start, char *end) {
  Token *token = (Token *)shirp_malloc(sizeof(Token));
  token->kind = kind;
  token->loc = start;
  token->len = (size_t)(end - start);
  token->next = NULL;

  return token;
}

static bool startswith(char *str, char *prefix) {
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

static size_t read_punct(char *p) {
  char *puncts[] = {"(", ")", "'"};
  for (unsigned int i = 0; i < sizeof(puncts) / sizeof(puncts[0]); i++) {
    if (startswith(p, puncts[i])) {
      return strlen(puncts[i]);
    }
  }
  return 0;
}

static bool included(char c, char *str) {
  for (unsigned int i = 0; i < strlen(str); i++) {
    if (c == str[i]) {
      return true;
    }
  }
  return false;
}

static bool is_ident_valid(char c) {
  return isdigit(c) || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
         included(c, "!$%&*+-./:<=>?@^_~");
}

static size_t read_ident(char *p, int *kind) {
  /*
  kind: 0(ident), 1(integer), 2(floating)
  */
  *kind = 1;
  size_t len = 0;
  while (is_ident_valid(*p)) {
    if (*kind == 1 && *p == '.') {
      *kind = 2;
    } else if (*kind == 2 && *p == '.') {
      *kind = 0;
    } else if (!isdigit(*p)) {
      *kind = 0;
    }
    len++;
    p++;
  }
  return len;
}

void tokenize(char *input) {
  Token head = {};
  Token *last_tok = &head;
  char *c = input;
  while (*c) {
    if (isspace(*c))
      c++;
    size_t punct_len = read_punct(c);
    if (punct_len > 0) {
      last_tok = last_tok->next = new_token(TOKEN_PUNCTUATOR, c, c + punct_len);
      c += punct_len;
      continue;
    }

    int tok_kind;
    size_t ident_len = read_ident(c, &tok_kind);
    if (ident_len > 0) {
      switch (tok_kind) {
      case 0:
        last_tok = last_tok->next = new_token(TOKEN_IDENT, c, c + ident_len);
        c += ident_len;
        break;
      case 1:
        last_tok = last_tok->next = new_token(TOKEN_INTEGER, c, c + ident_len);
        last_tok->val.int_val = strtol(c, &c, 10);
        break;
      case 2:
        last_tok = last_tok->next = new_token(TOKEN_FLOAT, c, c + ident_len);
        last_tok->val.float_val = (double)strtold(c, &c);
        break;
      }
      continue;
    }
    fprintf(stderr, "Error: invalid character: %c\n", *c);
    exit(EXIT_FAILURE);
  }
  cur = head.next;
}
