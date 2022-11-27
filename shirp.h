#ifndef __SHIRP_H__
#define __SHIRP_H__

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
bool match_anyof_tok(Token *tok, char *keywords[]);

typedef enum {
  UNDEF_TY,
  BOOL_TY,
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
    bool bool_val;
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
  ND_IF,
  ND_LAMBDA,
  ND_DEFINE,
  ND_PROCCALL,
} NodeKind;

typedef struct ASTNode ASTNode;
struct ASTNode {
  NodeKind kind;
  Token *tok;      // representative token
  ASTNode *caller; // procedure calls caller expression
  ASTNode *args;   // procedure calls hold arguments
  ASTNode *next;   // if this is an argument, next is needed
};

ASTNode *program();
ASTNode *expression();
void dump_tree(ASTNode *node);
char *get_command(ASTNode *node);
void free_ast(ASTNode *node);
void free_tokens(Token *tok);
void evaluate_ast(ASTNode *ast, bool make_childprocess);

/* Environment related */
typedef struct {
  char *key;
  size_t keylen;
  void *val;
} Entry;

typedef struct {
  size_t used;
  size_t capacity;
  Entry **buckets;
} HashTable;

typedef struct Frame Frame;
struct Frame {
  Frame *outer;
  HashTable *table;
  bool is_held;
};

Frame *push_new_frame(Frame *outer);
Frame *pop_frame(Frame *frame);
void frame_insert_obj(Frame *frame, char *key, size_t keylen, void *val);
void *frame_get_obj(Frame *frame, char *key, size_t keylen);

/* Evaluation Trees */
Obj *eval_ast(ASTNode *node);
void print_obj(Obj *obj);

/* util.c: Utility functions especially for debugging*/
void *shirp_malloc(size_t size);
void *shirp_calloc(size_t n, size_t size);
void *shirp_realloc(void *ptr, size_t size);
void verror_at(char *loc, char *fmt, va_list ap);
void tok_error_at(Token *tok, char *fmt, ...);
void debug_log(char *fmt, ...);

void dump_tokens(Token *tokens);
void dump_hashtable(HashTable *ht);

#endif