#include "shirp.h"

// offset given by `>> ` repl prompt
#define PROMPT_OFFSET 3

Token *cur;
int brackets_left = 0;
bool lexical_error = false;

// reports error with its position
static void verror_at(char *input, char *loc, char *fmt, va_list ap) {
  int offset = (int)(loc - input) + PROMPT_OFFSET;
  fprintf(stderr, "%*s^ ", offset, "");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
}

static void error_at(char *input, char *loc, char *fmt, ...) {
  lexical_error = true;
  va_list ap;
  va_start(ap, fmt);
  verror_at(input, loc, fmt, ap);
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

static bool is_delimiter(char c) {
  return isspace(c) || c == '\0' || included(c, "\n|()\";");
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

    if (is_delimiter(*c)) {
      if (*c == '(') {
        brackets_left++;
        last_tok = last_tok->next = new_token(TOKEN_DELIMITER, c, c + 1);
        c++;
        continue;
      } else if (*c == ')') {
        if (brackets_left <= 0) {
          error_at(input, c, "unexpected ')'");
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
            error_at(input, start - 1, "unterminated '|'");
            return last_tok;
            // error_at(start, "unclosed delimiter");
          }
          c++;
        }
        last_tok = last_tok->next = new_token(TOKEN_DELIMITER, start, c++);
        continue;
      }
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
    error_at(input, c, "invalid character: %c", *c);
    break;
  }
  return last_tok;
}
