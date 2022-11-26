#include "shirp.h"

static Token *prev;
Token *cur;
bool syntax_error = false;
// reports error with its position

static void error_at(Token *tok, char *fmt, ...) {
  fprintf(stderr, "parser error: \n");
  syntax_error = true;
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->loc, fmt, ap);
}

static ASTNode *new_ast_node(NodeKind kind, Token *tok) {
  ASTNode *node = (ASTNode *)shirp_malloc(sizeof(ASTNode));
  node->kind = kind;
  node->tok = tok;
  return node;
}

void read_next() {
  prev = cur;
  cur = cur->next;
}

static bool consume_lbr() {
  if (cur->kind == TOKEN_DELIMITER && match_tok(cur, "(")) {
    read_next();
    return true;
  }
  return false;
}

static bool consume_rbr() {
  if (cur->kind == TOKEN_DELIMITER && match_tok(cur, ")")) {
    read_next();
    return true;
  }
  return false;
}

void expect_rbr() {
  if (!consume_rbr())
    error_at(cur, "expected '('");
}

static bool consume(TokenKind kind) {
  if (cur->kind == kind) {
    read_next();
    return true;
  }
  return false;
}

ASTNode *expr() {
  if (consume(TOKEN_IDENT)) {
    debug_log("Identifier read");
    return new_ast_node(ND_IDENT, cur);
  } else if (consume(TOKEN_NUMBER)) {
    debug_log("Number read");
    return new_ast_node(ND_NUMBER, prev);
  } else if (consume_lbr()) {
    if (cur->kind != TOKEN_KEYWORD) {
      debug_log("Proc Call!");
      ASTNode *node = new_ast_node(ND_PROCCALL, cur);
      node->caller = expr();
      if (!node->caller)
        return NULL;
      ASTNode *args = NULL;
      ASTNode *last_arg = args;
      while (cur && !match_tok(cur, ")")) {
        ASTNode *arg = expr();
        if (!args) {
          args = last_arg = arg;
        } else
          last_arg = last_arg->next = arg;
      }
      node->args = args;
      if (consume_rbr()) {
        return node;
      }
    } else if (match_tok(cur, "if")) {
      debug_log("If statement!");
      ASTNode *node = new_ast_node(ND_IF, cur);
      read_next();
      ASTNode *test = expr();
      ASTNode *consequent = expr();
      ASTNode *alternate = NULL;
      if (!match_tok(cur, ")")) {
        alternate = expr();
      }
      expect_rbr();
      node->args = test;
      test->next = consequent;
      consequent->next = alternate;
      return node;
    }
  }
  error_at(cur, "unexpected token");
  return NULL;
}