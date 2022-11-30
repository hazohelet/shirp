#include "shirp.h"

int brackets_left = 0;
bool lexical_error = false;

extern Obj *true_obj;
extern Obj *false_obj;

static void error_at(char *loc, size_t len, char *fmt, ...) {
  lexical_error = true;
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, len, fmt, ap);
}

static Token *new_token(TokenKind kind, char *start, char *end) {
  Token *token = (Token *)shirp_malloc(sizeof(Token));
  token->kind = kind;
  token->loc = start;
  token->len = (size_t)(end - start);
  token->next = NULL;

  return token;
}

static bool included(char c, char *str) {
  for (unsigned int i = 0; i < strlen(str); i++) {
    if (c == str[i]) {
      return true;
    }
  }
  return false;
}

/* check whether the char is a delimiter or not */
static bool is_delimiter(char c) {
  return isspace(c) || c == '\0' || included(c, "\n|()\";");
}

/* check whether the char is valid as an identifier character */
static bool is_ident_valid(char c) {
  return isdigit(c) || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
         included(c, "!$%&*+-./:<=>?@^_~");
}

static bool is_sign(char c) { return included(c, "+-"); }

static size_t read_token(char *p, int *kind) {
  /*
  kind: 0(ident), 1(integer), 2(floating),
  */
  *kind = 1;
  size_t len = 0;
  if (is_sign(*p) && (isdigit(*(p + 1)) || *(p + 1) == '.')) {
    len++;
    p++;
  }
  while (is_ident_valid(*p)) {
    if (*kind == 1 && *p == '.' && (isdigit(*(p + 1)) || isdigit(*(p - 1)))) {
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

/* whether the identifier is recognized as keyword */
bool is_keyword(Token *token) {
  if (token->kind != TOKEN_IDENT) {
    return false;
  }
  static char *keywords[] = {
      "if",          "cond",    "case",        "else",         "and",
      "or",          "when",    "unless",      "let",          "let*",
      "letrec",      "letrec*", "let-values",  "let*-values",  "begin",
      "do",          "delay",   "delay-force", "parameterize", "guard",
      "case-lambda", "lambda"};
  for (size_t i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
    if (token->len == strlen(keywords[i]) &&
        !strncmp(token->loc, keywords[i], token->len)) {
      return true;
    }
  }
  return false;
}

Token *handle_sharp(char *c, char **cref) {
  char *head = c;
  while (*c && is_ident_valid(*c)) {
    c++;
  }
  size_t len = (size_t)(c - head);
  /* TODO: impl other sharp features */
  Token *tok = new_token(TOKEN_NUMBER, head, c);
  if (match_str(*cref, "t", len) || match_str(*cref, "true", len))
    tok->obj = true_obj;
  else if (match_str(*cref, "f", len) || match_str(*cref, "false", len))
    tok->obj = false_obj;

  *cref += len;
  return tok;
}

/*
  input(char *): the head of the input string
  last_tok(Token *): the last toekn that has been scanned
*/
Token *tokenize(char *input, Token *last_tok) {
  char *c = input;
  while (*c) {
    if (isspace(*c) || *c == '\n' || *c == '\r')
      c++;
    if (*c == ';') {
      while (*c != '\n' && *c != '\0')
        c++;
      continue;
    }

    if (*c == '#') {
      last_tok = last_tok->next = handle_sharp(++c, &c);
      continue;
    }
    if (*c == '\'') {
      debug_log("quote scanned");
      last_tok = last_tok->next = new_token(TOKEN_QUOTE, c, c + 1);
      c++;
      continue;
    }

    if (is_delimiter(*c)) {
      if (*c == '(') {
        brackets_left++;
        last_tok = last_tok->next = new_token(TOKEN_DELIMITER, c, c + 1);
        c++;
        continue;
      } else if (*c == ')') {
        if (brackets_left <= 0) {
          error_at(c, 1, "unexpected ')'");
          return last_tok;
        }
        brackets_left--;
        last_tok = last_tok->next = new_token(TOKEN_DELIMITER, c, c + 1);
        c++;
        continue;
      } else if (*c == '|') {
        char *start = ++c;
        while (*c != '|') {
          if (*c == '\0') {
            error_at(start - 1, 1, "unterminated '|'");
            return last_tok;
          }
          c++;
        }
        last_tok = last_tok->next = new_token(TOKEN_DELIMITER, start, c++);
        continue;
      }
      continue;
    }

    int tok_kind;
    size_t ident_len = read_token(c, &tok_kind);
    if (ident_len > 0) {
      switch (tok_kind) {
      case 0:
        last_tok = last_tok->next = new_token(TOKEN_IDENT, c, c + ident_len);
        c += ident_len;
        break;
      case 1:
        last_tok = last_tok->next = new_token(TOKEN_NUMBER, c, c + ident_len);
        last_tok->obj = new_obj(INT_TY);
        last_tok->obj->num_val.int_val = strtol(c, &c, 10);
        break;
      case 2:
        last_tok = last_tok->next = new_token(TOKEN_NUMBER, c, c + ident_len);
        last_tok->obj = new_obj(FLOAT_TY);
        last_tok->obj->num_val.float_val = (double)strtold(c, &c);
        break;
      }
      if (is_keyword(last_tok)) {
        last_tok->kind = TOKEN_KEYWORD;
      }
      continue;
    }
    error_at(c, 1, "invalid character: %c", *c);
    break;
  }
  return last_tok;
}
