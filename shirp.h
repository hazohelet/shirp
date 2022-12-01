#ifndef __SHIRP_H__
#define __SHIRP_H__

#define __STDC_FORMAT_MACROS
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TOMBSTONE ((void *)-1)

typedef enum {
  TOKEN_IDENT,
  TOKEN_IMMEDIATE,
  TOKEN_STRING,
  TOKEN_RESERVED,
  TOKEN_DELIMITER,
  TOKEN_KEYWORD,
  TOKEN_PERIOD,
  TOKEN_QUOTE,
} TokenKind;

typedef enum {
  UNDEF_TY,   // undefined
  BOOL_TY,    // bool
  INT_TY,     // integer (64bit)
  FLOAT_TY,   // float (64bit)
  CHAR_TY,    // character
  STRING_TY,  // string
  SYMBOL_TY,  // symbol
  LAMBDA_TY,  // lambda closure
  BUILTIN_TY, // built-in function
  CONS_TY,    // cons cell
} ObjType;

typedef struct Token Token;
typedef struct Frame Frame;
typedef struct ASTNode ASTNode;
typedef struct Obj Obj;
typedef struct WorkList WorkList;
typedef struct GC GC;

struct Token {
  TokenKind kind; // token kind
  ObjType typ;    // type of immediate
  union {
    bool bool_val;
    int64_t int_val;
    double float_val;
  } val;
  char *loc;   // holds reference to the source string
  size_t len;  // length of the literal
  Token *next; // next token
};

Token *tokenize(char *input, Token *last_token);
bool match_str(char *str, char *keyword, size_t len);
bool match_tok(Token *tok, char *keyword);
bool match_anyof_tok(Token *tok, char *keywords[]);

struct Obj {
  ObjType typ; // value type of object
  /* Number Value */
  union {
    bool bool_val;
    int64_t int_val;
    double float_val;
  } num_val;
  /* String or Symbol Value */
  char *str_val;
  size_t str_len;
  /* Lambda attributes */
  Frame *saved_env;
  ASTNode *lambda_ast;
  /* List attributes */
  Obj *car;
  Obj *cdr;
};
Obj *new_obj(ObjType typ);

typedef enum {
  ND_IDENT,
  ND_IMMEDIATE,
  ND_STRING,
  ND_QUOTE,
  ND_SYMBOL,
  ND_IF,
  ND_LAMBDA,
  ND_DEFINE,
  ND_PROCCALL,
} NodeKind;

struct ASTNode {
  NodeKind kind;
  Token *tok;      // representative token
  ASTNode *caller; // procedure calls caller expression; lambda hold its body
  ASTNode *args; // procedure calls hold arguments; lambda holds its parameters
  ASTNode
      *listarg;  // lambda holds list args with <formal> without () or with `.`
  ASTNode *next; // if this is an argument, next is needed
  bool is_tail_call; // for proper tail call optimization
};

ASTNode *program();
ASTNode *expression();
void dump_tree(ASTNode *node);
char *get_command(ASTNode *node);
void free_ast(ASTNode *node);
void free_tokens(Token *tok);
void evaluate_ast(ASTNode *ast, bool make_childprocess);
void mark_tail_calls(ASTNode *node, bool is_in_tail_context);

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

struct Frame {
  Frame *outer;
  HashTable *table;
  bool is_held;
};

HashTable *new_hash_table(size_t capacity);
Entry *hashtable_get(HashTable *ht, char *key, size_t keylen);
void hashtable_delete(HashTable *ht, char *key, size_t keylen);
void hashtable_insert(HashTable *ht, char *key, size_t keylen, void *val);
Frame *push_frame(Frame *frame, Frame *outer);
Frame *push_new_frame(Frame *outer);
Frame *pop_frame(Frame *frame);
void frame_insert_obj(Frame *frame, char *key, size_t keylen, void *val);
void *frame_get_obj(Frame *frame, char *key, size_t keylen);
Frame *copy_frame(Frame *dst, Frame *src);
Frame *copied_environment(Frame *frame);

/* Evaluation Trees */
Obj *eval_ast(ASTNode *node);
void print_obj(Obj *obj);
void println_obj(Obj *obj);
bool is_list(Obj *obj);

/* util.c: Utility functions especially for debugging*/
void *shirp_malloc(size_t size);
void *shirp_calloc(size_t n, size_t size);
void *shirp_realloc(void *ptr, size_t size);
void verror_at(char *loc, size_t size, char *fmt, va_list ap);
void tok_error_at(Token *tok, char *fmt, ...);
void debug_log(char *fmt, ...);
void debug_printf(char *fmt, ...);

void dump_tokens(Token *tokens);
void dump_hashtable(HashTable *ht);
void dump_env(Frame *env);

void free_tokens(Token *tok);
void free_ast(ASTNode *ast);
void free_obj(Obj *obj);

/* Garbage Collector */

struct WorkList {
  Obj *obj;
  WorkList *next;
};

struct GC {
  WorkList *head;
  WorkList *tail;
  HashTable *marked_table;
  size_t size;
};

void GC_init();
void GC_collect(Obj *obj);
void GC_mark_and_sweep();
void GC_dump();
void hashtable_insert_ptr(HashTable *ht, void *ptr, void *val);
void *hashtable_get_ptr(HashTable *ht, void *ptr);
void hashtable_delete_ptr(HashTable *ht, void *ptr);

void shirp_init();

#endif