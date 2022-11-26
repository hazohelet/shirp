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
bool match(char *str, char *keyword, size_t len);
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
  NODE_PIPE,
  NODE_OUTPUT_REDIRECT,
  NODE_INPUT_REDIRECT,
  NODE_EXECUNIT,
  NODE_SEQUENCE,
  NODE_BACKGROUND_EXEC
} ASTNodeKind;

typedef struct ASTNode ASTNode;
struct ASTNode {
  ASTNodeKind kind;   // indicate what operation needs to be performed
  ASTNode *lhs, *rhs; // for binary and unary operations
  Token *tok;         // representative token
};

ASTNode *expr();
void dump_tree(ASTNode *node);
char *get_command(ASTNode *node);
void free_ast(ASTNode *node);
void free_tokens(Token *tok);
void evaluate_ast(ASTNode *ast, bool make_childprocess);

int bsh_cd(char **);
int bsh_help(char **);
int bsh_exit(char **);
int bsh_bg(char **);
int bsh_fg(char **);
int bsh_jobs(char **);
int launch_builtin_command_if_possible(char **argv);

/* util.c: Utility functions especially for debugging*/
void *shirp_malloc(size_t size);

#endif