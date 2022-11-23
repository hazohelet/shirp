#ifndef __SHIRP_H__
#define __SHIRP_H__

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum {
  TOKEN_IDENT,
  TOKEN_INTEGER,
  TOKEN_FLOAT,
  TOKEN_STRING,
  TOKEN_RESERVED,
  TOKEN_PUNCTUATOR,
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;
  char *loc;
  size_t len;
  Token *next;
  union {
    int64_t int_val;
    double float_val;
  } val; // hold value for numbers
};

void tokenize(char *input);
void dump_tokens(Token *tokens);

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