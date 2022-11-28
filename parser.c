#include "shirp.h"

static Token *prev;
Token *cur;
bool syntax_error = false;
// reports error with its position

void tok_error_at(Token *tok, char *fmt, ...) {
  syntax_error = true;
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->loc, tok->len, fmt, ap);
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

static bool is_definition_start() {
  Token *peek = cur->next;
  /*
  static char *kw[] = {"define", "define-values", "define-record-type",
                       "define-syntax"};
                       */
  return match_tok(cur, "(") && peek && match_tok(peek, "define");
}

ASTNode *command_or_definition() {
  if (is_definition_start())
    return definition();
  return command();
}

ASTNode *command() { return expression(); }

ASTNode *expression() {
  if (consume(TOKEN_IDENT)) {
    debug_log("Identifier parsed: %.*s", prev->len, prev->loc);
    return new_ast_node(ND_IDENT, prev);
  } else if (consume(TOKEN_NUMBER)) {
    debug_log("Number parsed: %.*s", prev->len, prev->loc);
    return new_ast_node(ND_NUMBER, prev);
  } else if (consume_lbr()) {
    if (cur->kind != TOKEN_KEYWORD) {
      debug_log("Proc Call parsed!: %.*s", cur->len, cur->loc);
      ASTNode *node = new_ast_node(ND_PROCCALL, cur);
      node->caller = expression();
      if (!node->caller)
        return NULL;
      node->args = NULL;
      ASTNode *last_arg = NULL;
      while (cur && !match_tok(cur, ")")) {
        ASTNode *arg = expression();
        if (!node->args)
          node->args = last_arg = arg;
        else
          last_arg = last_arg->next = arg;
      }
      if (consume_rbr()) {
        return node;
      }
    } else if (consume_tok("if")) {
      debug_log("If statement!");
      ASTNode *node = new_ast_node(ND_IF, cur);
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
    } else if (consume_tok("lambda")) {
      debug_log("lambda is parsed!");
      /* read formals */
      expect_lbr();
      ASTNode *node = new_ast_node(ND_LAMBDA, cur);
      ASTNode *last_arg = NULL;
      debug_log("lambda formals are parsed!");
      while (consume(TOKEN_IDENT)) {
        debug_log("Identifier parsed: %.*s", prev->len, prev->loc);
        if (!node->args)
          node->args = last_arg = new_ast_node(ND_IDENT, prev);
        else
          last_arg = last_arg->next = new_ast_node(ND_IDENT, prev);
      }
      expect_rbr();
      /* read <body> -> <definition>* <expression>+ */
      debug_log("lambda body is parsed!");
      ASTNode *last_body = NULL;
      dump_tokens(cur);
      if (!is_definition_start()) {
        debug_log("no defs");
      } else {
        debug_log("theres defs");
      }
      while (is_definition_start()) {
        debug_log("lambda definitions are parsed!");
        if (!node->caller)
          node->caller = last_body = definition();
        else
          last_body = last_body->next = definition();
      }
      /* <expression> */
      debug_log("lambda body-expression is parsed");
      if (!node->caller)
        node->caller = last_body = expression();
      else
        last_body = last_body->next = expression();

      /* <expression>* */
      while (!consume_rbr()) {
        if (!node->caller)
          node->caller = last_body = expression();
        else
          last_body = last_body->next = expression();
      }
      debug_log("lambda has been parsed");
      return node;
    }
  }
  tok_error_at(cur, "unexpected token");
  return NULL;
}

ASTNode *definition() {
  debug_log("definition is parsed");
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