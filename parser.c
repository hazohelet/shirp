#include "shirp.h"

static Token *prev;
Token *cur;
bool syntax_error = false;
// reports error with its position

#define RETURN_IF_ERROR()                                                      \
  if (syntax_error)                                                            \
    return NULL;

void tok_error_at(Token *tok, char *fmt, ...) {
  syntax_error = true;
  va_list ap;
  va_start(ap, fmt);
  if (!tok) {
    verror_at(prev->loc, prev->len, fmt, ap);
    return;
  }
  verror_at(tok->loc, tok->len, fmt, ap);
}

static ASTNode *new_ast_node(NodeKind kind, Token *tok) {
  ASTNode *node = (ASTNode *)shirp_calloc(1, sizeof(ASTNode));
  node->kind = kind;
  node->tok = tok;
  return node;
}

void read_next() {
  if (!cur) {
    prev = cur;
    return;
  }
  prev = cur;
  cur = cur->next;
}

static bool consume_lbr() {
  if (cur && cur->kind == TOKEN_DELIMITER && match_tok(cur, "(")) {
    read_next();
    return true;
  }
  return false;
}

static bool consume_rbr() {
  if (cur && cur->kind == TOKEN_DELIMITER && match_tok(cur, ")")) {
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

#define EXPECT_LBR()                                                           \
  expect_lbr();                                                                \
  RETURN_IF_ERROR()

void expect_rbr() {
  if (!consume_rbr())
    tok_error_at(cur, "expected ')'");
}

#define EXPECT_RBR()                                                           \
  expect_rbr();                                                                \
  RETURN_IF_ERROR()

static bool match_tokenkind(TokenKind kind) { return cur && cur->kind == kind; }

static bool consume(TokenKind kind) {
  if (match_tokenkind(kind)) {
    read_next();
    return true;
  }
  return false;
}

ASTNode *command_or_definition();
ASTNode *command();
ASTNode *definition();
ASTNode *body();
ASTNode *quoted_datum();
ASTNode *expression();
ASTNode *identifier();
ASTNode *number();

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
  RETURN_IF_ERROR()
  if (match_tokenkind(TOKEN_IDENT)) {
    return identifier();
  } else if (match_tokenkind(TOKEN_NUMBER)) {
    return number();
  } else if (consume(TOKEN_QUOTE)) {
    return quoted_datum();
  } else if (consume_lbr()) {
    if (cur->kind != TOKEN_KEYWORD) {
      if (match_tok(cur, "quote")) {
        read_next();
        ASTNode *quotes = quoted_datum();
        EXPECT_RBR()
        return quotes;
      }
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
      EXPECT_RBR()
      node->args = test;
      test->next = consequent;
      consequent->next = alternate;
      return node;
    } else if (consume_tok("lambda")) {
      debug_log("lambda is parsed!");
      /* read formals */
      EXPECT_LBR()
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
      EXPECT_RBR()
      node->caller = body();
      EXPECT_RBR()
      debug_log("lambda has been parsed");
      return node;
    } else if (consume_tok("let")) {
      /* ( let ( `(<identifier> <expression>)`* ) <body> )*/
      debug_log("let is parsed!");
      EXPECT_LBR()
      ASTNode *proccall = new_ast_node(ND_PROCCALL, cur);
      ASTNode *lambda = new_ast_node(ND_LAMBDA, cur);
      proccall->caller = lambda;
      ASTNode *last_param = NULL;
      ASTNode *last_arg = NULL;
      while (consume_lbr()) {
        debug_log("<binding spec> parsed");
        ASTNode *ident = identifier();
        ASTNode *expr = expression();
        EXPECT_RBR();
        if (!proccall->args) {
          lambda->args = last_param = ident;
          proccall->args = last_arg = expr;
        } else {
          last_param = last_param->next = ident;
          last_arg = last_arg->next = expr;
        }
      }
      EXPECT_RBR()
      lambda->caller = body();
      EXPECT_RBR()
      debug_log("let has been parsed");
      return proccall;
    }
  }
  tok_error_at(cur, "unexpected token");
  return NULL;
}

ASTNode *definition() {
  debug_log("definition is parsed");
  EXPECT_LBR()
  if (consume_tok("define")) {
    if (consume(TOKEN_IDENT)) {
      ASTNode *node = new_ast_node(ND_DEFINE, prev);
      node->caller = new_ast_node(ND_IDENT, prev);
      node->args = expression();
      RETURN_IF_ERROR()
      EXPECT_RBR()
      return node;
    } else if (consume_lbr()) {
      /* (define (<identifier> <def formals>) <body>) */
      debug_log("(define (<identifier> <def formals>) <body>)");
      ASTNode *name = identifier();
      ASTNode *def = new_ast_node(ND_DEFINE, prev);
      ASTNode *lambda = new_ast_node(ND_LAMBDA, prev);
      debug_log("def lambda name is %.*s", name->tok->len, name->tok->loc);
      def->caller = name;
      def->args = lambda;
      /* <def formals>) <body>) */
      /* <def formals> -> <identifier>* | <identifier>* . <identifier> */
      ASTNode *last_arg = NULL;

      while (match_tokenkind(TOKEN_IDENT)) {
        if (!lambda->args)
          lambda->args = last_arg = identifier();
        else
          last_arg = last_arg->next = identifier();
      }

      ASTNode *listarg = NULL;
      if (consume(TOKEN_PERIOD)) {
        listarg = identifier();
      }
      lambda->listarg = listarg;
      EXPECT_RBR()
      lambda->caller = body();
      EXPECT_RBR()
      return def;
    }
  }
  tok_error_at(cur, "expected identifier");
  return NULL;
}

/* <body> -> <definition>* <expression>+ */
ASTNode *body() {
  debug_log("body is parsed!");
  ASTNode *res = NULL;
  ASTNode *last_body = NULL;
  dump_tokens(cur);
  /* <definition *> */
  if (!is_definition_start()) {
    debug_log("NO define observed");
  } else {
    debug_log("THERE is define observed");
  }
  while (is_definition_start()) {
    debug_log("body-definitions are parsed!");
    if (!res)
      res = last_body = definition();
    else
      last_body = last_body->next = definition();
  }
  /* <expression> */
  debug_log("body-expression is parsed");
  if (!res)
    res = last_body = expression();
  else
    last_body = last_body->next = expression();

  /* <expression>* */
  while (!match_tok(cur, ")")) {
    if (!res)
      res = last_body = expression();
    else
      last_body = last_body->next = expression();
  }
  debug_log("body has been parsed");
  return res;
}

ASTNode *identifier() {
  if (consume(TOKEN_IDENT)) {
    debug_log("Identifier parsed: %.*s", prev->len, prev->loc);
    return new_ast_node(ND_IDENT, prev);
  }
  tok_error_at(cur, "expected identifier");
  return NULL;
}

ASTNode *number() {
  if (consume(TOKEN_NUMBER)) {
    debug_log("Number parsed: %.*s", prev->len, prev->loc);
    return new_ast_node(ND_NUMBER, prev);
  }
  tok_error_at(cur, "expected number");
  return NULL;
}

ASTNode *quoted_datum() {
  debug_log("quotation parsed");
  if (consume_lbr()) {
    ASTNode *node = new_ast_node(ND_QUOTE, prev);
    ASTNode *last_datum = NULL;
    while (!match_tok(cur, ")")) {
      if (!node->args)
        node->args = last_datum = quoted_datum();
      else
        last_datum = last_datum->next = quoted_datum();
    }
    EXPECT_RBR()
    return node;
  }
  if (match_tokenkind(TOKEN_NUMBER)) {
    return number();
  } else {
    debug_log("quoted datum is symbol");
    if (!cur) {
      tok_error_at(cur, "unterminated quote");
      return NULL;
    }
    read_next();
    debug_log("Quoted datum parsed: %.*s", prev->len, prev->loc);
    return new_ast_node(ND_SYMBOL, prev);
  }
}