#ifndef __SHIRP_H__
#define __SHIRP_H__

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum {
  TOKEN_IDENT,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_RESERVED,
  TOKEN_DELIMITER,
  TOKEN_KEYWORD,
  TOKEN_PERIOD,
} TokenKind;

typedef struct Token Token;
typedef struct Obj Obj;

struct Token {
  TokenKind kind; // token kind
  char *loc;
  size_t len;
  Token *next;
  Obj *obj; // value of the number tokens
};

Token *tokenize(char *input, Token *last_token);
bool match_str(char *str, char *keyword, size_t len);
bool match_tok(Token *tok, char *keyword);
void dump_tokens(Token *tokens);

typedef enum {
  INT_TY,
  FLOAT_TY,
  CHAR_TY,
  STRING_TY,
  LAMBDA_TY,
  LIST_TY,
  QUOTE_TY
} ObjType;

struct Obj {
  ObjType typ; // value type of object
  /* Number Value */
  union {
    int64_t int_val;
    double float_val;
  } num_val;
  /* String Value */
  char *str_val;
  /* Lambda attributes */
  char **params;
  Obj *body;
  /* List attributes */
  Obj *car;
  Obj *cdr;
};
Obj *new_obj(ObjType typ);

typedef enum {
  ND_IDENT,
  ND_NUMBER,
  ND_LAMBDA,
  ND_DEFINE,
  ND_PROCCALL,
} NodeKind;

typedef struct ASTNode ASTNode;
struct ASTNode {
  NodeKind kind;
  Token *tok;    // representative token
  ASTNode *args; // procedure calls hold arguments
  ASTNode *next; // if this is an argument, next is needed
};

ASTNode *expr();
void dump_tree(ASTNode *node);
char *get_command(ASTNode *node);
void free_ast(ASTNode *node);
void free_tokens(Token *tok);
void evaluate_ast(ASTNode *ast, bool make_childprocess);

/* Evaluation Trees */
Obj *eval_ast(ASTNode *node);
void print_obj(Obj *obj);

/* util.c: Utility functions especially for debugging*/
void *shirp_malloc(size_t size);
void verror_at(char *loc, char *fmt, va_list ap);
void debug_log(char *fmt, ...);

#endif