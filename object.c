#include "shirp.h"

extern Frame *env;
bool eval_error = false;

Obj *true_obj = &(Obj){BOOL_TY, {true}, NULL, NULL, NULL, NULL, NULL};
Obj *false_obj = &(Obj){BOOL_TY, {false}, NULL, NULL, NULL, NULL, NULL};
Obj *nillist = &(Obj){CONS_TY, {0}, NULL, NULL, NULL, NULL, NULL};

#define RETURN_IF_ERROR()                                                      \
  if (eval_error)                                                              \
    return NULL;

void print_obj(Obj *obj) {
  if (!obj) {
    fprintf(stderr, "NULL");
    return;
  }
  switch (obj->typ) {
  case UNDEF_TY:
    fprintf(stderr, "#<undef>");
    break;
  case BOOL_TY:
    fprintf(stderr, "%s", obj->num_val.bool_val ? "#t" : "#f");
    break;
  case INT_TY:
    fprintf(stderr, "%" PRId64 "", obj->num_val.int_val);
    break;
  case FLOAT_TY:
    fprintf(stderr, "%lf", obj->num_val.float_val);
    break;
  case LAMBDA_TY:
    fprintf(stderr, "#<closure>");
    break;
  case CONS_TY:
    fprintf(stderr, "(");
    while (obj && obj->cdr && obj != nillist) {
      Obj *car = obj->car;
      Obj *cdr = obj->cdr;
      print_obj(car);
      if (obj->cdr != nillist)
        fprintf(stderr, " ");
      obj = cdr;
    }
    if (obj == nillist) {
      fprintf(stderr, ")");
      return;
    }
    fprintf(stderr, ". ");
    print_obj(obj);
    fprintf(stderr, ")");
    break;
  default:
    fprintf(stderr, "Eval Error");
  }
}

bool is_list(Obj *obj) {
  if (!obj)
    return false;
  if (obj == nillist)
    return true;
  if (obj->typ != CONS_TY)
    return false;
  return is_list(obj->cdr);
}

void println_obj(Obj *obj) {
  print_obj(obj);
  fprintf(stderr, "\n");
}

Obj *new_obj(ObjType typ) {
  Obj *obj = (Obj *)shirp_calloc(1, sizeof(Obj));
  obj->typ = typ;
  return obj;
}

Obj *new_int_obj(int64_t val) {
  Obj *obj = new_obj(INT_TY);
  obj->num_val.int_val = val;
  return obj;
}

Obj *new_float_obj(double val) {
  Obj *obj = new_obj(FLOAT_TY);
  obj->num_val.float_val = val;
  return obj;
}

Obj *copy_value_obj(Obj *obj) {
  Obj *new_obj = (Obj *)shirp_calloc(1, sizeof(Obj));
  new_obj->typ = obj->typ;
  new_obj->num_val = obj->num_val;
  return new_obj;
}

double get_float_val(Obj *obj) {
  if (obj->typ == INT_TY)
    return (double)obj->num_val.int_val;
  return obj->num_val.float_val;
}

void Obj_add_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->num_val.int_val = op1->num_val.int_val + op2->num_val.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = FLOAT_TY;
    dst->num_val.float_val = op1_val + op2_val;
  }
}

void Obj_sub_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->num_val.int_val = op1->num_val.int_val - op2->num_val.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = FLOAT_TY;
    dst->num_val.float_val = op1_val - op2_val;
  }
}

void Obj_mul_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->num_val.int_val = op1->num_val.int_val * op2->num_val.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = FLOAT_TY;
    dst->num_val.float_val = op1_val * op2_val;
  }
}

void Obj_div_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->num_val.int_val = op1->num_val.int_val / op2->num_val.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = FLOAT_TY;
    dst->num_val.float_val = op1_val / op2_val;
  }
}

void Obj_mod_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->num_val.int_val = op1->num_val.int_val % op2->num_val.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = FLOAT_TY;
    dst->num_val.float_val = fmod(op1_val, op2_val);
  }
}

void Obj_lt_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = BOOL_TY;
    dst->num_val.bool_val = op1->num_val.int_val < op2->num_val.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = BOOL_TY;
    dst->num_val.bool_val = op1_val < op2_val;
  }
}

void Obj_le_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = BOOL_TY;
    dst->num_val.bool_val = op1->num_val.int_val <= op2->num_val.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = BOOL_TY;
    dst->num_val.bool_val = op1_val <= op2_val;
  }
}

void Obj_eq_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = BOOL_TY;
    dst->num_val.bool_val = op1->num_val.int_val == op2->num_val.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = BOOL_TY;
    dst->num_val.bool_val = op1_val == op2_val;
  }
}

size_t get_argc(ASTNode *args) {
  size_t argc = 0;
  while (args) {
    argc++;
    args = args->next;
  }
  return argc;
}

/* repr_tok is for printing error info */
Obj *call_user_procedure(Token *repr_tok, Obj *proc, ASTNode *args) {
  debug_log("calling user procedure: `%.*s`", repr_tok->len, repr_tok->loc);
  if (proc->typ != LAMBDA_TY) {
    tok_error_at(repr_tok, "not a procedure");
    eval_error = true;
    return NULL;
  }
  const size_t proc_argc = get_argc(proc->lambda_ast->args);
  const size_t passed_argc = get_argc(args);
  if (proc_argc != passed_argc) {
    tok_error_at(repr_tok, "wrong number of arguments: expects %ld, got %ld",
                 proc_argc, passed_argc);
    eval_error = true;
    return NULL;
  }

  /* copied because the pointer of the saved environment needs to point other
   * frames, especially in recursive calls */
  Frame *exec_env =
      copied_environment(proc->saved_env); // retrieve the saved environment
  exec_env = push_frame(exec_env, env);    // push frame on the stack

  ASTNode *proc_arg = proc->lambda_ast->args;
  /* set up arguments to new frame */
  while (proc_arg) {
    frame_insert_obj(exec_env, proc_arg->tok->loc, proc_arg->tok->len,
                     eval_ast(args));
    RETURN_IF_ERROR()
    proc_arg = proc_arg->next;
    args = args->next;
  }
  /* Environment has to come here because procedure arguments values shall be
   * evaluated in the previous environment */
  env = exec_env;

  Obj *res = NULL;
  ASTNode *body = proc->lambda_ast->caller;
  while (body) {
    res = eval_ast(body);
    body = body->next;
  }
  env = pop_frame(env); // pop the execution environment
  return res;
}

Obj *handle_proc_call(ASTNode *node) {
  if (match_tok(node->tok, "+")) {
    debug_log("+ Handled!");
    Obj *result = new_int_obj(0);
    ASTNode *arg = node->args;
    while (arg) {
      Obj *op2 = eval_ast(arg);
      RETURN_IF_ERROR()
      Obj_add_operation(result, result, op2);
      arg = arg->next;
    }
    return result;
  } else if (match_tok(node->tok, "-")) {
    debug_log("- Handled!");
    Obj *result = copy_value_obj(eval_ast(node->args));
    ASTNode *arg = node->args->next;
    while (arg) {
      Obj *op2 = eval_ast(arg);
      RETURN_IF_ERROR()
      Obj_sub_operation(result, result, op2);
      arg = arg->next;
    }
    return result;
  } else if (match_tok(node->tok, "*")) {
    debug_log("* evaled!");
    Obj *result = copy_value_obj(eval_ast(node->args));
    ASTNode *arg = node->args->next;
    while (arg) {
      Obj *op2 = eval_ast(arg);
      RETURN_IF_ERROR()
      Obj_mul_operation(result, result, op2);
      arg = arg->next;
    }
    return result;
  } else if (match_tok(node->tok, "<")) {
    debug_log("< Handled!");
    size_t argc = get_argc(node->args);
    if (argc < 2) {
      tok_error_at(node->tok,
                   "wrong number of arguments: expects 2 or more, got %ld",
                   get_argc(node->args));
      eval_error = true;
      return NULL;
    } else if (argc > 2)
      return false_obj;
    Obj *result = copy_value_obj(eval_ast(node->args)); // this is op1
    Obj *op2 = eval_ast(node->args->next);              // this is op2
    RETURN_IF_ERROR()
    Obj_lt_operation(result, result, op2);
    return result;
  } else if (match_tok(node->tok, "<=")) {
    debug_log("<= Handled!");
    size_t argc = get_argc(node->args);
    if (argc < 2) {
      tok_error_at(node->tok,
                   "wrong number of arguments: expects 2 or more, got %ld",
                   get_argc(node->args));
      eval_error = true;
      return NULL;
    } else if (argc > 2)
      return false_obj;
    Obj *result = copy_value_obj(eval_ast(node->args)); // this is op1
    Obj *op2 = eval_ast(node->args->next);              // this is op2
    RETURN_IF_ERROR()
    Obj_le_operation(result, result, op2);
    return result;
  } else if (match_tok(node->tok, "div")) {
    debug_log("`div` Handled!");
    size_t argc = get_argc(node->args);
    if (argc != 2) {
      tok_error_at(node->tok,
                   "wrong number of arguments: expects 2 or more, got %ld",
                   get_argc(node->args));
      eval_error = true;
      return NULL;
    }
    Obj *result = copy_value_obj(eval_ast(node->args)); // this is op1
    Obj *op2 = eval_ast(node->args->next);              // this is op2
    RETURN_IF_ERROR()
    Obj_div_operation(result, result, op2);
    return result;
  } else if (match_tok(node->tok, "remainder")) {
    debug_log("`remainder` Handled!");
    size_t argc = get_argc(node->args);
    if (argc != 2) {
      tok_error_at(node->tok,
                   "wrong number of arguments: expects 2 or more, got %ld",
                   get_argc(node->args));
      eval_error = true;
      return NULL;
    }
    Obj *result = copy_value_obj(eval_ast(node->args)); // this is op1
    Obj *op2 = eval_ast(node->args->next);              // this is op2
    RETURN_IF_ERROR()
    Obj_mod_operation(result, result, op2);
    return result;
  } else if (match_tok(node->tok, "=")) {
    debug_log("= Handled!");
    size_t argc = get_argc(node->args);
    if (argc != 2) {
      tok_error_at(node->tok,
                   "wrong number of arguments: expects 2 or more, got %ld",
                   get_argc(node->args));
      eval_error = true;
      return NULL;
    }
    Obj *result = copy_value_obj(eval_ast(node->args)); // this is op1
    Obj *op2 = eval_ast(node->args->next);              // this is op2
    RETURN_IF_ERROR()
    Obj_eq_operation(result, result, op2);
    return result;
  } else if (match_tok(node->tok, "cons")) {
    debug_log("`cons` evaled!");
    size_t argc = get_argc(node->args);
    if (argc != 2) {
      tok_error_at(node->tok, "wrong number of arguments: expects 2, got %ld",
                   get_argc(node->args));
      eval_error = true;
      return NULL;
    }
    Obj *res = new_obj(CONS_TY);
    Obj *op1 = eval_ast(node->args);
    Obj *op2 = eval_ast(node->args->next);
    RETURN_IF_ERROR()
    res->car = op1;
    res->cdr = op2;
    return res;
  } else if (match_tok(node->tok, "list")) {
    debug_log("`list` evaled!");
    Obj *head = nillist;
    Obj *tail = nillist;
    ASTNode *arg = node->args;
    while (arg) {
      Obj *cell = new_obj(CONS_TY);
      cell->car = eval_ast(arg);
      cell->cdr = nillist;
      RETURN_IF_ERROR()
      if (head == nillist) {
        head = tail = cell;
      } else {
        tail = tail->cdr = cell;
      }
      arg = arg->next;
    }
    return head;
  } else if (match_tok(node->tok, "car")) {
    debug_log("car evaled!");
    size_t argc = get_argc(node->args);
    if (argc != 1) {
      tok_error_at(node->tok, "wrong number of arguments: expects 1, got %ld",
                   get_argc(node->args));
      eval_error = true;
      return NULL;
    }
    Obj *cell = eval_ast(node->args);
    RETURN_IF_ERROR()
    if (cell == nillist || cell->typ != CONS_TY) {
      tok_error_at(node->tok, "car: expects a cons cell");
      eval_error = true;
      return NULL;
    }
    return cell->car;
  } else if (match_tok(node->tok, "cdr")) {
    debug_log("cdr evaled!");
    size_t argc = get_argc(node->args);
    if (argc != 1) {
      tok_error_at(node->tok, "wrong number of arguments: expects 1, got %ld",
                   get_argc(node->args));
      eval_error = true;
      return NULL;
    }
    Obj *cell = eval_ast(node->args);
    RETURN_IF_ERROR()
    if (cell == nillist || cell->typ != CONS_TY) {
      tok_error_at(node->tok, "car: expects a cons cell");
      eval_error = true;
      return NULL;
    }
    return cell->cdr;
  }

  Obj *caller = eval_ast(node->caller);
  if (!caller)
    return NULL;
  return call_user_procedure(node->tok, caller, node->args);
}

static bool is_not_false(Obj *obj) {
  return !(obj->typ == BOOL_TY && obj->num_val.bool_val == false);
}

Obj *eval_if(ASTNode *node) {
  ASTNode *test = node->args;
  Obj *test_val = eval_ast(test);
  RETURN_IF_ERROR()
  ASTNode *consequent = test->next;
  ASTNode *alternate = consequent->next;

  if (is_not_false(test_val))
    return eval_ast(consequent);
  return eval_ast(alternate);
}

void handle_definition(ASTNode *node) {
  debug_log("definition handled!");
  frame_insert_obj(env, node->caller->tok->loc, node->caller->tok->len,
                   eval_ast(node->args));
}

Obj *handle_lambda(ASTNode *node) {
  debug_log("lambda evaled!");
  Obj *obj = new_obj(LAMBDA_TY);
  obj->lambda_ast = node;
  obj->saved_env = copied_environment(env);
  return obj;
}

Obj *eval_ast(ASTNode *node) {
  if (!node) {
    return new_obj(UNDEF_TY);
  }
  switch (node->kind) {
  case ND_IDENT:
    debug_log("handle identifier `%.*s` in the following environment",
              node->tok->len, node->tok->loc);
    dump_env(env);
    Obj *obj = frame_get_obj(env, node->tok->loc, node->tok->len);
    if (obj)
      return obj;
    else {
      eval_error = true;
      tok_error_at(node->tok, "undefined variable: %.*s", node->tok->len,
                   node->tok->loc);
      return NULL;
    }
  case ND_NUMBER:
    return node->tok->obj;
  case ND_IF:
    return eval_if(node);
  case ND_PROCCALL:
    return handle_proc_call(node);
  case ND_LAMBDA:
    return handle_lambda(node);
  case ND_DEFINE:
    handle_definition(node);
    break;
  }
  return NULL;
}