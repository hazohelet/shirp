#include "shirp.h"

static Token *prev;
Token *cur;
bool syntax_error = false;
// reports error with its position

#define RETURN_IF_ERROR()                                                      \
  if (syntax_error)                                                            \
    return NULL;

void read_next() {
  if (!cur) {
    prev = cur;
    return;
  }
  prev = cur;
  cur = cur->next;
}

void tok_error_at(Token *tok, char *fmt, ...) {
  syntax_error = true;
  va_list ap;
  va_start(ap, fmt);
  if (!tok) {
    verror_at(prev->loc, prev->len, fmt, ap);
    return;
  }
  verror_at(tok->loc, tok->len, fmt, ap);
  read_next();
}

static ASTNode *new_ast_node(NodeKind kind, Token *tok) {
  ASTNode *node = (ASTNode *)shirp_calloc(1, sizeof(ASTNode));
  node->kind = kind;
  node->tok = tok;
  return node;
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

static ASTNode *copied_ast(ASTNode *node) {
  ASTNode *new_node = (ASTNode *)shirp_calloc(1, sizeof(ASTNode));
  memcpy(new_node, node, sizeof(ASTNode));
  return new_node;
}

ASTNode *command_or_definition();
ASTNode *command();
ASTNode *definition();
ASTNode *body();
ASTNode *sequence();
ASTNode *datum();
ASTNode *expression();
ASTNode *identifier();
ASTNode *number();
ASTNode *string();

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
  } else if (match_tokenkind(TOKEN_IMMEDIATE)) {
    return number();
  } else if (match_tokenkind(TOKEN_STRING)) {
    return string();
  } else if (consume(TOKEN_QUOTE)) {
    return datum();
  } else if (consume_lbr()) {
    if (cur->kind != TOKEN_KEYWORD) {
      if (match_tok(cur, "quote")) {
        read_next();
        ASTNode *quotes = datum();
        EXPECT_RBR()
        return quotes;
      }
      debug_log("Proc Call parsed!: %.*s", cur->len, cur->loc);
      ASTNode *node = new_ast_node(ND_PROCCALL, cur);
      node->caller = expression();
      RETURN_IF_ERROR()
      node->args = NULL;
      ASTNode *last_arg = NULL;
      while (cur && !match_tok(cur, ")")) {
        ASTNode *arg = expression();
        RETURN_IF_ERROR()
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
    } else if (consume_tok("cond")) {
      debug_log("cond is parsed");
      ASTNode *node = new_ast_node(ND_COND, prev);
      EXPECT_LBR()
      if (consume_tok("else")) { // (cond (else <sequence>))
        node->listarg = sequence();
        EXPECT_RBR()
        EXPECT_RBR()
        return node;
      }

      // at least one cond clause is promised
      ASTNode *test = expression();
      ASTNode *seq = NULL;
      if (match_tok(cur, ")"))
        seq = copied_ast(test);
      else
        seq = sequence();
      node->caller = test;
      node->args = seq;
      EXPECT_RBR()
      while (consume_lbr()) {
        if (consume_tok("else")) {
          node->listarg = sequence();
          EXPECT_RBR()
          EXPECT_RBR()
          return node;
        }
        test = test->next = expression();
        if (match_tok(cur, ")"))
          seq = seq->next = copied_ast(test);
        else
          seq = seq->next = sequence();
        EXPECT_RBR()
      }
      EXPECT_RBR()
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
  tok_error_at(cur, "unexpected token in expression");
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

/* <sequence> -> <expression>+ */
ASTNode *sequence() {
  debug_log("sequence is parsed");
  ASTNode *seq = new_ast_node(ND_SEQUENCE, cur);
  ASTNode *head = expression();
  seq->args = head;
  ASTNode *node = head;
  while (cur && !match_tok(cur, ")")) {
    node->next = expression();
    RETURN_IF_ERROR()
    node = node->next;
  }
  return seq;
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
  /* <sequence> */
  debug_log("body-sequence is parsed");
  if (!res)
    res = last_body = sequence();
  else
    last_body = last_body->next = sequence();
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
  if (consume(TOKEN_IMMEDIATE)) {
    debug_log("Number parsed: %.*s", prev->len, prev->loc);
    return new_ast_node(ND_IMMEDIATE, prev);
  }
  tok_error_at(cur, "expected number");
  return NULL;
}

ASTNode *string() {
  if (consume(TOKEN_STRING)) {
    debug_log("String parsed: %.*s", prev->len, prev->loc);
    return new_ast_node(ND_STRING, prev);
  }
  tok_error_at(cur, "expected string");
  return NULL;
}

ASTNode *list();
ASTNode *simple_datum();

ASTNode *datum() {
  debug_log("datum is parsed");
  if (match_tok(cur, "(")) {
    return list();
  }
  return simple_datum();
}

ASTNode *list() {
  debug_log("datum_list is parsed");
  EXPECT_LBR()
  ASTNode *lst = new_ast_node(ND_QUOTE, cur);
  ASTNode *last = NULL;
  while (!match_tok(cur, ")") && !match_tokenkind(TOKEN_PERIOD)) {
    if (!lst->args)
      lst->args = last = datum();
    else
      last = last->next = datum();
  }
  if (!match_tok(prev, "(") && consume(TOKEN_PERIOD)) {
    debug_log("datum_list is a cons cell");
    ASTNode *dotted = datum();
    lst->listarg = dotted;
  }
  EXPECT_RBR()
  return lst;
}

ASTNode *simple_datum() {
  debug_log("simple datum parsed");
  if (!cur) {
    tok_error_at(cur, "unterminated quote");
    return NULL;
  }
  if (match_tokenkind(TOKEN_IMMEDIATE)) {
    return number();
  } else if (match_tokenkind(TOKEN_IDENT) || match_tokenkind(TOKEN_KEYWORD)) {
    debug_log("quoted datum is symbol");
    read_next();
    return new_ast_node(ND_SYMBOL, prev);
  } else if (match_tokenkind(TOKEN_STRING)) {
    return string();
  }
  tok_error_at(cur, "unexpected token in datum");
  return NULL;
}

void mark_tail_calls(ASTNode *node, bool is_in_tail_context) {
  if (!node)
    return;
  switch (node->kind) {
  case ND_LAMBDA:;
    ASTNode *body = node->caller;
    while (body && body->next) {
      mark_tail_calls(body, false);
      body = body->next;
    }
    mark_tail_calls(body, true);
    dump_tokens(body->tok);
    return;
  case ND_PROCCALL:
    node->is_tail_call = is_in_tail_context;
    if (is_in_tail_context) {
      debug_log("tail call detected: %.*s", node->tok->len, node->tok->loc);
    }
    mark_tail_calls(node->caller, false);
    ASTNode *arg = node->args;
    while (arg) {
      mark_tail_calls(arg, false);
      arg = arg->next;
    }
    return;
  case ND_QUOTE:;
    ASTNode *item = node->args;
    while (item) {
      mark_tail_calls(item, false);
      item = item->next;
    }
    return;
  case ND_IF:;
    ASTNode *test = node->args;
    ASTNode *consequent = test->next;
    ASTNode *alternate = consequent->next;
    mark_tail_calls(test, false);
    mark_tail_calls(consequent, is_in_tail_context);
    mark_tail_calls(alternate, is_in_tail_context);
    return;
  case ND_COND:;
    ASTNode *else_sequence = node->listarg;
    mark_tail_calls(else_sequence, is_in_tail_context);
    break;
  case ND_SEQUENCE:;
    ASTNode *expr = node->args;
    while (expr && expr->next) {
      mark_tail_calls(expr, false);
      expr = expr->next;
    }
    mark_tail_calls(expr, is_in_tail_context);
    break;
  case ND_DEFINE:;
    mark_tail_calls(node->args, false);
    return;
  case ND_IDENT:
  case ND_IMMEDIATE:
  case ND_STRING:
  case ND_SYMBOL:
    return;
  }
}