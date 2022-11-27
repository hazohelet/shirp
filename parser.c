#include "shirp.h"

static Token *prev;
Token *cur;
bool syntax_error = false;
// reports error with its position

void tok_error_at(Token *tok, char *fmt, ...) {
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

static bool consume_tok(char *str) {
  if (match_tok(cur, str)) {
    read_next();
    return true;
  }
  return false;
}

void expect_lbr() {
  if (!consume_lbr())
    tok_error_at(cur, "expected '('");
}

void expect_rbr() {
  if (!consume_rbr())
    tok_error_at(cur, "expected ')'");
}

static bool consume(TokenKind kind) {
  if (cur->kind == kind) {
    read_next();
    return true;
  }
  return false;
}

ASTNode *command_or_definition();
ASTNode *command();
ASTNode *definition();
ASTNode *expression();

ASTNode *program() { return command_or_definition(); }

ASTNode *command_or_definition() {
  Token *peek = cur->next;
  char *kw[] = {"define", "define-values", "define-record-type",
                "define-syntax"};
  if (peek && match_anyof_tok(peek, kw))
    return definition();
  return command();
}

ASTNode *command() { return expression(); }

ASTNode *expression() {
  if (consume(TOKEN_IDENT)) {
    debug_log("Identifier read: %.*s", prev->len, prev->loc);
    return new_ast_node(ND_IDENT, prev);
  } else if (consume(TOKEN_NUMBER)) {
    debug_log("Number read");
    return new_ast_node(ND_NUMBER, prev);
  } else if (consume_lbr()) {
    if (cur->kind != TOKEN_KEYWORD) {
      debug_log("Proc Call!");
      ASTNode *node = new_ast_node(ND_PROCCALL, cur);
      node->caller = expression();
      if (!node->caller)
        return NULL;
      ASTNode *args = NULL;
      ASTNode *last_arg = args;
      while (cur && !match_tok(cur, ")")) {
        ASTNode *arg = expression();
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
      ASTNode *test = expression();
      ASTNode *consequent = expression();
      ASTNode *alternate = NULL;
      if (!match_tok(cur, ")")) {
        alternate = expression();
      }
      expect_rbr();
      node->args = test;
      test->next = consequent;
      consequent->next = alternate;
      return node;
    }
  }
  tok_error_at(cur, "unexpected token");
  return NULL;
}

ASTNode *definition() {
  expect_lbr();
  if (consume_tok("define")) {
    if (consume(TOKEN_IDENT)) {
      ASTNode *node = new_ast_node(ND_DEFINE, prev);
      node->caller = new_ast_node(ND_IDENT, prev);
      node->args = expression();
      expect_rbr();
      return node;
    }
  }
  tok_error_at(cur, "expected identifier");
  return NULL;
}