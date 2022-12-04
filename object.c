#include "shirp.h"

extern Frame *env;
bool eval_error = false;
bool side_effect = false;

/* Some of the common values are pre-defined for memory efficiency */
Obj *true_obj = &(Obj){BOOL_TY, {true}, NULL, NULL};
Obj *false_obj = &(Obj){BOOL_TY, {false}, NULL, NULL};
Obj *nillist = &(Obj){CONS_TY, {0}, NULL, NULL};
Obj *undefined_obj = &(Obj){UNDEF_TY, {0}, NULL, NULL};

#define RETURN_IF_ERROR()                                                      \
  if (eval_error)                                                              \
    return NULL;

/* `print_obj` does not print '\n' at the end for recursive calls */
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
    fprintf(stderr, "%s", obj->exclusive.bool_val ? "#t" : "#f");
    break;
  case INT_TY:
    fprintf(stderr, "%" PRId64 "", obj->exclusive.int_val);
    break;
  case FLOAT_TY:
    fprintf(stderr, "%lf", obj->exclusive.float_val);
    break;
  case LAMBDA_TY:
    fprintf(stderr, "#<closure>");
    break;
  case CONS_TY: // cons cells are assumed not to be circular
    fprintf(stderr, "(");
    while (obj && obj->cdr && obj != nillist) {
      Obj *car = obj->exclusive.car;
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
  case SYMBOL_TY:
    fprintf(stderr, "%s", obj->exclusive.str_val);
    break;
  case CHAR_TY:
    fprintf(stderr, "#\\%s", obj->exclusive.str_val);
    break;
  case STRING_TY:
    fprintf(stderr, "\"%s\"", obj->exclusive.str_val);
    break;
  case BUILTIN_TY:
    fprintf(stderr, "#<builtin-closure>");
    break;
  }
}

bool is_number(Obj *obj) {
  return obj && (obj->typ == INT_TY || obj->typ == FLOAT_TY);
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
  GC_register(obj);
  return obj;
}

Obj *bool_obj(bool val) { return val ? true_obj : false_obj; }

Obj *new_int_obj(int64_t val) {
  Obj *obj = new_obj(INT_TY);
  obj->exclusive.int_val = val;
  return obj;
}

Obj *new_float_obj(double val) {
  Obj *obj = new_obj(FLOAT_TY);
  obj->exclusive.float_val = val;
  return obj;
}

Obj *copy_value_obj(Obj *obj) {
  Obj *copied = new_obj(obj->typ);
  memcpy(copied, obj, sizeof(Obj));
  return copied;
}

Obj *new_string_obj(char *str, size_t len) {
  Obj *obj = new_obj(STRING_TY);
  obj->exclusive.str_val = (char *)shirp_malloc(len + 1);
  memcpy(obj->exclusive.str_val, str, len);
  obj->exclusive.str_val[len] = '\0';
  return obj;
}

double get_float_val(Obj *obj) {
  if (obj->typ == INT_TY)
    return (double)obj->exclusive.int_val;
  return obj->exclusive.float_val;
}

void Obj_add_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->exclusive.int_val = op1->exclusive.int_val + op2->exclusive.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = FLOAT_TY;
    dst->exclusive.float_val = op1_val + op2_val;
  }
}

void Obj_sub_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->exclusive.int_val = op1->exclusive.int_val - op2->exclusive.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = FLOAT_TY;
    dst->exclusive.float_val = op1_val - op2_val;
  }
}

void Obj_mul_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->exclusive.int_val = op1->exclusive.int_val * op2->exclusive.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = FLOAT_TY;
    dst->exclusive.float_val = op1_val * op2_val;
  }
}

void Obj_div_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->exclusive.int_val = op1->exclusive.int_val / op2->exclusive.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = FLOAT_TY;
    dst->exclusive.float_val = op1_val / op2_val;
  }
}

void Obj_mod_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->exclusive.int_val = op1->exclusive.int_val % op2->exclusive.int_val;
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    dst->typ = FLOAT_TY;
    dst->exclusive.float_val = fmod(op1_val, op2_val);
  }
}

Obj *Obj_lt_operation(Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    return bool_obj(op1->exclusive.int_val < op2->exclusive.int_val);
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    return bool_obj(op1_val < op2_val);
  }
}

Obj *Obj_le_operation(Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    return bool_obj(op1->exclusive.int_val <= op2->exclusive.int_val);
  } else {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op2);
    return bool_obj(op1_val <= op2_val);
  }
}

Obj *string_equal_obj(Obj *op1, Obj *op2) {
  return bool_obj(strcmp(op1->exclusive.str_val, op2->exclusive.str_val) == 0);
}

Obj *and_obj(Obj *op1, Obj *op2) {
  return bool_obj(op1->exclusive.int_val && op2->exclusive.int_val);
}

Obj *or_obj(Obj *op1, Obj *op2) {
  return bool_obj(op1->exclusive.int_val || op2->exclusive.int_val);
}

// = and eq?
Obj *eq_obj(Obj *op1, Obj *op2) {
  if (op1 == op2)
    return bool_obj(true);

  if (op1->typ == INT_TY && op2->typ == INT_TY)
    return bool_obj(op1->exclusive.int_val == op2->exclusive.int_val);
  if (is_number(op1) && is_number(op2)) {
    double op1_val = get_float_val(op1);
    double op2_val = get_float_val(op1);
    return bool_obj(op1_val == op2_val);
  }
  if (op1->typ == SYMBOL_TY && op2->typ == SYMBOL_TY)
    return string_equal_obj(op1, op2);
  if (op1->typ == STRING_TY && op2->typ == STRING_TY)
    return string_equal_obj(op1, op2);
  if (op1->typ == CHAR_TY && op2->typ == CHAR_TY)
    return string_equal_obj(op1, op2);
  return bool_obj(false);
}

// equal?
Obj *equal_obj(Obj *op1, Obj *op2) {
  Obj *eq_res = eq_obj(op1, op2);
  if (eq_res == true_obj)
    return true_obj;

  if (op1->typ != op2->typ)
    return false_obj;
  if (op1 == nillist || op2 == nillist)
    return false_obj;

  if (op1->typ == CONS_TY)
    return and_obj(equal_obj(op1->exclusive.car, op2->exclusive.car),
                   equal_obj(op1->cdr, op2->cdr));
  return false_obj;
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
Obj *call_user_procedure(Token *repr_tok, Obj *proc, ASTNode *args,
                         bool is_tail_call) {
  debug_log("calling user procedure: `%.*s`", repr_tok->len, repr_tok->loc);
  if (proc->typ != LAMBDA_TY) {
    tok_error_at(repr_tok, "not a procedure");
    eval_error = true;
    return NULL;
  }
  if (is_tail_call)
    debug_log("TAIL CALL!!!!!!!");
  else
    debug_log("NOT a tail call");
  const size_t proc_argc = get_argc(proc->lambda_ast->args);
  const size_t passed_argc = get_argc(args);
  if (proc_argc != passed_argc) {
    tok_error_at(repr_tok, "wrong number of arguments: expects %ld, got %ld",
                 proc_argc, passed_argc);
    eval_error = true;
    return NULL;
  }

  Frame *exec_env;
  /* tail call optimization */
  /* If is a tail call, the current frame will be used again
     If not, push a new frame on the environment */
  if (is_tail_call)
    exec_env = env;
  else
    exec_env = push_new_frame(env);

  ASTNode *proc_arg = proc->lambda_ast->args;
  /* set up arguments to new frame */
  Frame *arg_frame = push_new_frame(NULL);
  while (proc_arg) {
    /* register to the frame other than `env` because it would interfere with
       the coming argument evaluations */
    frame_insert_obj(arg_frame, proc_arg->tok->loc, proc_arg->tok->len,
                     eval_ast(args));
    RETURN_IF_ERROR()
    proc_arg = proc_arg->next;
    args = args->next;
  }
  env = exec_env;
  /* Before adding args, add the saved env info to the frame */
  env = copy_frame(env, proc->exclusive.saved_env);
  /* Add arguments to the current environment */
  env = copy_frame(env, arg_frame);
  /* frame for argument is a temporary one, shall be discarded */
  pop_frame(arg_frame);

  /* the last value is the result of procedure call */
  Obj *res = NULL;
  ASTNode *body = proc->lambda_ast->caller;
  while (body) {
    res = eval_ast(body);
    body = body->next;
  }
  /* If not a tail call, we shall pop the frame for the execution when `ret` */
  if (!is_tail_call)
    env = pop_frame(env);
  return res;
}

bool match_name(char *name, char *name2) { return strcmp(name, name2) == 0; }

#define REQUIRE_ARGC_EQ(tok, args, num)                                        \
  if (get_argc(args) != num) {                                                 \
    tok_error_at(tok, "wrong number of arguments: expects %ld, got %ld", num,  \
                 get_argc(args));                                              \
    eval_error = true;                                                         \
    return NULL;                                                               \
  }

#define REQUIRE_ARGC_GE(tok, args, num)                                        \
  if (get_argc(args) < num) {                                                  \
    tok_error_at(tok,                                                          \
                 "wrong number of arguments: expects at least %ld, got %ld",   \
                 num, get_argc(args));                                         \
    eval_error = true;                                                         \
    return NULL;                                                               \
  }

#define REQUIRE_OBJ_NUM_TYPE(tok, obj)                                         \
  if (!is_number(obj)) {                                                       \
    tok_error_at(tok, "argument expected to be a number, but got `%s`",        \
                 type_name(obj->typ));                                         \
    eval_error = true;                                                         \
    return NULL;                                                               \
  }

#define REQUIRE_OBJ_TYPE(tok, obj, type)                                       \
  if (obj->typ != type) {                                                      \
    tok_error_at(tok, "argument expected to be of type `%s`, but got `%s`",    \
                 type_name(type), type_name(obj->typ));                        \
    eval_error = true;                                                         \
    return NULL;                                                               \
  }

Obj *handle_builtin(Token *tok, char *name, ASTNode *args) {
  if (match_name(name, "cons")) {
    debug_log("<BUILTIN cons> evaled!");
    REQUIRE_ARGC_EQ(tok, args, 2);
    Obj *res = new_obj(CONS_TY);
    Obj *op1 = eval_ast(args);
    Obj *op2 = eval_ast(args->next);
    RETURN_IF_ERROR()
    res->exclusive.car = op1;
    res->cdr = op2;
    return res;
  } else if (match_name(name, "null?")) {
    debug_log("<BUILTIN null?> evaled!");
    REQUIRE_ARGC_EQ(tok, args, 1)
    Obj *val = eval_ast(args);
    RETURN_IF_ERROR()
    return bool_obj(val == nillist);
  } else if (match_name(name, "+")) {
    debug_log("+ Handled!");
    Obj *result = new_int_obj(0);
    ASTNode *arg = args;
    while (arg) {
      Obj *op2 = eval_ast(arg);
      RETURN_IF_ERROR()
      REQUIRE_OBJ_NUM_TYPE(arg->tok, op2)
      Obj_add_operation(result, result, op2);
      arg = arg->next;
    }
    return result;
  } else if (match_name(name, "-")) {
    debug_log("- Handled!");
    REQUIRE_ARGC_GE(tok, args, 1)
    Obj *op1 = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->tok, op1);
    Obj *result = copy_value_obj(op1);
    if (get_argc(args) == 1) {
      Obj_sub_operation(result, new_int_obj(0), result);
      return result;
    }
    ASTNode *arg = args->next;
    while (arg) {
      Obj *op2 = eval_ast(arg);
      RETURN_IF_ERROR()
      REQUIRE_OBJ_NUM_TYPE(arg->tok, op2);
      Obj_sub_operation(result, result, op2);
      arg = arg->next;
    }
    return result;
  } else if (match_name(name, "*")) {
    debug_log("* evaled!");
    Obj *result = new_int_obj(1);
    ASTNode *arg = args;
    while (arg) {
      Obj *op2 = eval_ast(arg);
      RETURN_IF_ERROR()
      REQUIRE_OBJ_NUM_TYPE(arg->tok, op2)
      Obj_mul_operation(result, result, op2);
      arg = arg->next;
    }
    return result;
  } else if (match_name(name, "<")) {
    debug_log("< Handled!");
    REQUIRE_ARGC_GE(tok, args, 2)
    Obj *op1 = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->tok, op1)
    Obj *op2 = eval_ast(args->next);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->next->tok, op2)
    Obj *result = Obj_lt_operation(op1, op2);
    ASTNode *arg = args->next->next;
    op1 = op2;
    while (arg) {
      op2 = eval_ast(arg);
      RETURN_IF_ERROR()
      REQUIRE_OBJ_NUM_TYPE(arg->tok, op2)
      result = and_obj(result, Obj_lt_operation(op1, op2));
      op1 = op2;
      arg = arg->next;
    }
    return result;
  } else if (match_name(name, ">")) {
    debug_log("> Handled!");
    REQUIRE_ARGC_GE(tok, args, 2)
    Obj *op1 = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->tok, op1)
    Obj *op2 = eval_ast(args->next);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->next->tok, op2)
    Obj *result = Obj_lt_operation(op2, op1);
    ASTNode *arg = args->next->next;
    op1 = op2;
    while (arg) {
      op2 = eval_ast(arg);
      RETURN_IF_ERROR()
      REQUIRE_OBJ_NUM_TYPE(arg->tok, op2)
      result = and_obj(result, Obj_lt_operation(op2, op1));
      op1 = op2;
      arg = arg->next;
    }
    return result;
  } else if (match_name(name, "<=")) {
    debug_log("<= Handled!");
    REQUIRE_ARGC_GE(tok, args, 2)
    Obj *op1 = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->tok, op1)
    Obj *op2 = eval_ast(args->next);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->next->tok, op2)
    Obj *result = Obj_le_operation(op1, op2);
    ASTNode *arg = args->next->next;
    op1 = op2;
    while (arg) {
      op2 = eval_ast(arg);
      RETURN_IF_ERROR()
      REQUIRE_OBJ_NUM_TYPE(arg->tok, op2)
      result = and_obj(result, Obj_le_operation(op1, op2));
      op1 = op2;
      arg = arg->next;
    }
    return result;
  } else if (match_name(name, ">=")) {
    debug_log(">= Handled!");
    REQUIRE_ARGC_GE(tok, args, 2)
    Obj *op1 = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->tok, op1)
    Obj *op2 = eval_ast(args->next);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->next->tok, op2)
    Obj *result = Obj_le_operation(op2, op1);
    ASTNode *arg = args->next->next;
    op1 = op2;
    while (arg) {
      op2 = eval_ast(arg);
      RETURN_IF_ERROR()
      REQUIRE_OBJ_NUM_TYPE(arg->tok, op2)
      result = and_obj(result, Obj_le_operation(op2, op1));
      op1 = op2;
      arg = arg->next;
    }
    return result;
  } else if (match_name(name, "div") || match_name(name, "/")) {
    debug_log("`div` Handled!");
    REQUIRE_ARGC_EQ(tok, args, 2)
    Obj *op1 = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->tok, op1)
    Obj *result = copy_value_obj(op1);
    Obj *op2 = eval_ast(args->next);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->next->tok, op2)
    Obj_div_operation(result, result, op2);
    return result;
  } else if (match_name(name, "remainder")) {
    debug_log("`remainder` Handled!");
    REQUIRE_ARGC_EQ(tok, args, 2)
    Obj *op1 = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->tok, op1)
    Obj *result = copy_value_obj(op1);
    Obj *op2 = eval_ast(args->next);
    REQUIRE_OBJ_NUM_TYPE(args->next->tok, op2)
    RETURN_IF_ERROR()
    Obj_mod_operation(result, result, op2);
    return result;
  } else if (match_name(name, "=")) {
    debug_log("= Handled!");
    REQUIRE_ARGC_GE(tok, args, 2)
    Obj *op1 = eval_ast(args);
    RETURN_IF_ERROR()
    Obj *op2 = eval_ast(args->next);
    RETURN_IF_ERROR()
    Obj *result = eq_obj(op1, op2);
    ASTNode *arg = args->next->next;
    op1 = op2;
    while (arg) {
      op2 = eval_ast(arg);
      RETURN_IF_ERROR()
      result = and_obj(result, eq_obj(op1, op2));
      op1 = op2;
      arg = arg->next;
    }
    return result;
  } else if (match_name(name, "eq?")) {
    debug_log("eq? Handled!");
    REQUIRE_ARGC_EQ(tok, args, 2)
    Obj *op1 = eval_ast(args);
    RETURN_IF_ERROR()
    Obj *op2 = eval_ast(args->next);
    RETURN_IF_ERROR()
    return eq_obj(op1, op2);
  } else if (match_name(name, "list")) {
    debug_log("`list` evaled!");
    Obj *head = nillist;
    Obj *tail = nillist;
    ASTNode *arg = args;
    while (arg) {
      Obj *cell = new_obj(CONS_TY);
      cell->exclusive.car = eval_ast(arg);
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
  } else if (match_tok(tok, "car")) {
    debug_log("car evaled!");
    REQUIRE_ARGC_EQ(tok, args, 1)
    Obj *cell = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_TYPE(args->tok, cell, CONS_TY)
    return cell->exclusive.car;
  } else if (match_name(name, "cdr")) {
    debug_log("cdr evaled!");
    REQUIRE_ARGC_EQ(tok, args, 1)
    Obj *cell = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_TYPE(args->tok, cell, CONS_TY)
    return cell->cdr;
  } else if (match_name(name, "and")) { // argc >= 0
    Obj *result = true_obj;
    ASTNode *arg = args;
    while (arg) {
      result = eval_ast(arg);
      RETURN_IF_ERROR()
      if (result == false_obj)
        break;
      arg = arg->next;
    }
    return result;
  } else if (match_name(name, "or")) { // argc >= 0
    Obj *result = false_obj;
    ASTNode *arg = args;
    while (arg) {
      result = eval_ast(arg);
      RETURN_IF_ERROR()
      if (result != false_obj)
        break;
      arg = arg->next;
    }
    return result;
  } else if (match_name(name, "pair?")) { // argc == 1
    REQUIRE_ARGC_EQ(tok, args, 1);
    Obj *arg = eval_ast(args);
    RETURN_IF_ERROR()
    return bool_obj(arg->typ == CONS_TY && arg != nillist);
  } else if (match_name(name, "list?")) {
    REQUIRE_ARGC_EQ(tok, args, 1);
    Obj *arg = eval_ast(args);
    RETURN_IF_ERROR()
    return bool_obj(is_list(arg));
  } else if (match_name(name, "number?")) {
    REQUIRE_ARGC_EQ(tok, args, 1);
    Obj *arg = eval_ast(args);
    RETURN_IF_ERROR()
    return bool_obj(is_number(arg));
  } else if (match_name(name, "symbol?")) {
    REQUIRE_ARGC_EQ(tok, args, 1);
    Obj *arg = eval_ast(args);
    RETURN_IF_ERROR()
    return bool_obj(arg && arg->typ == SYMBOL_TY);
  } else if (match_name(name, "equal?")) { // argc == 2
    REQUIRE_ARGC_EQ(tok, args, 2)
    Obj *op1 = eval_ast(args);
    RETURN_IF_ERROR()
    Obj *op2 = eval_ast(args->next);
    RETURN_IF_ERROR()
    return equal_obj(op1, op2);
  } else if (match_name(name, "sqrt")) {
    REQUIRE_ARGC_EQ(tok, args, 1)
    Obj *op1 = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_NUM_TYPE(args->tok, op1);
    double val = sqrt(get_float_val(op1));
    if (op1->typ == FLOAT_TY)
      return new_float_obj(val);

    if ((int)val == val)
      return new_int_obj((int)val);
    else
      return new_float_obj(val);

    return NULL;
  } else if (match_name(name, "load")) {
    REQUIRE_ARGC_EQ(tok, args, 1)
    Obj *filename_obj = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_TYPE(args->tok, filename_obj, STRING_TY)
    bool success = load_file(filename_obj->exclusive.str_val);
    RETURN_IF_ERROR()
    return success ? true_obj : NULL;
  } else if (match_name(name, "even?")) {
    REQUIRE_ARGC_EQ(tok, args, 1)
    Obj *operand = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_TYPE(args->tok, operand, INT_TY)
    return bool_obj((operand->exclusive.int_val & 1) == 0);
  } else if (match_name(name, "odd?")) {
    REQUIRE_ARGC_EQ(tok, args, 1)
    Obj *operand = eval_ast(args);
    RETURN_IF_ERROR()
    REQUIRE_OBJ_TYPE(args->tok, operand, INT_TY)
    return bool_obj((operand->exclusive.int_val & 1) != 0);
  }

  tok_error_at(tok, "unknown builtin function: %s", name);
  return NULL;
}

Obj *handle_proc_call(ASTNode *node) {
  Obj *caller = eval_ast(node->caller);
  RETURN_IF_ERROR()
  if (caller->typ == BUILTIN_TY)
    return handle_builtin(node->caller->tok, caller->exclusive.str_val,
                          node->args);

  if (node->is_tail_call)
    return call_user_procedure(node->tok, caller, node->args, true);
  else
    return call_user_procedure(node->tok, caller, node->args, false);
}

static bool is_not_false(Obj *obj) {
  return !(obj->typ == BOOL_TY && obj->exclusive.bool_val == false);
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

Obj *eval_quote(ASTNode *node) {
  debug_log("quote evaled!");

  Obj *head = nillist;
  Obj *tail = nillist;
  ASTNode *arg = node->args;
  while (arg) {
    Obj *cell = new_obj(CONS_TY);
    cell->exclusive.car = eval_ast(arg);
    cell->cdr = nillist;
    RETURN_IF_ERROR()
    if (head == nillist) {
      head = tail = cell;
    } else {
      tail = tail->cdr = cell;
    }
    arg = arg->next;
  }
  if (node->listarg)
    tail->cdr = eval_ast(node->listarg);

  return head;
}

Obj *handle_lambda(ASTNode *node, Obj *placeholder) {
  debug_log("lambda evaled!");
  Obj *obj = placeholder ? placeholder : new_obj(LAMBDA_TY);
  obj->lambda_ast = node;
  obj->exclusive.saved_env = copied_environment(env);
  return obj;
}

Obj *handle_definition(ASTNode *node) {
  debug_log("definition handled!");
  side_effect = true;
  Obj *value;
  if (node->args->kind == ND_LAMBDA) {
    /* make a placeholder for lambda so that that the evaluated lambda
     * saved_env holds the information of itself */
    value = new_obj(LAMBDA_TY);
    /* register to the environment before the lambda evaluation */
    frame_insert_obj(env, node->caller->tok->loc, node->caller->tok->len,
                     value);
    handle_lambda(node->args, value);
    RETURN_IF_ERROR()
    return NULL;
  } else {
    value = eval_ast(node->args);
    RETURN_IF_ERROR()
    frame_insert_obj(env, node->caller->tok->loc, node->caller->tok->len,
                     value);
    return NULL;
  }
}

Obj *eval_symbol(ASTNode *node) {
  debug_log("symbol evaled!");
  Obj *obj = new_obj(SYMBOL_TY);
  size_t len = node->tok->len;
  obj->exclusive.str_val = (char *)shirp_malloc(len + 1);
  memcpy(obj->exclusive.str_val, node->tok->loc, len);
  obj->exclusive.str_val[len] = '\0';
  return obj;
}

Obj *eval_immediate(Token *tok) {
  debug_log("immediate evaled!");
  Obj *obj = NULL;
  switch (tok->typ) {
  case INT_TY:
    obj = new_obj(INT_TY);
    obj->exclusive.int_val = tok->val.int_val;
    return obj;
  case FLOAT_TY:;
    obj = new_obj(FLOAT_TY);
    obj->exclusive.float_val = tok->val.float_val;
    return obj;
  case BOOL_TY:
    return bool_obj(tok->val.bool_val);
  default:
    tok_error_at(tok, "unknown immediate type");
  }
  return NULL;
}

Obj *eval_sequence(ASTNode *node) {
  debug_log("sequence evaled!");
  ASTNode *arg = node->args;
  Obj *result = NULL;
  while (arg) {
    result = eval_ast(arg);
    RETURN_IF_ERROR()
    arg = arg->next;
  }
  return result;
}

Obj *handle_set(ASTNode *node) {
  debug_log("set! handled!");
  side_effect = true;
  Obj *obj = eval_ast(node->caller);
  RETURN_IF_ERROR()
  Obj *new_val = eval_ast(node->args);
  RETURN_IF_ERROR()
  memcpy(obj, new_val, sizeof(Obj));
  if (obj->typ == LAMBDA_TY) {
    obj->exclusive.saved_env = push_new_frame(NULL);
    copy_frame(obj->exclusive.saved_env, obj->exclusive.saved_env);
  }

  return obj;
}

Obj *eval_cond(ASTNode *node) {
  ASTNode *test = node->caller;
  ASTNode *sequence = node->args;
  while (test && sequence) {
    Obj *test_val = eval_ast(test);
    RETURN_IF_ERROR()
    if (is_not_false(test_val)) {
      Obj *res = eval_ast(sequence);
      RETURN_IF_ERROR()
      return res;
    }
    test = test->next;
    sequence = sequence->next;
  }
  if (node->listarg) // else sequence
    return eval_sequence(node->listarg);
  return undefined_obj;
}

Obj *eval_ast(ASTNode *node) {
  if (!node) {
    return undefined_obj;
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
  case ND_IMMEDIATE:
    return eval_immediate(node->tok);
  case ND_STRING:
    return new_string_obj(node->tok->loc, node->tok->len);
  case ND_QUOTE:
    return eval_quote(node);
  case ND_SYMBOL:
    return eval_symbol(node);
  case ND_IF:
    return eval_if(node);
  case ND_COND:
    return eval_cond(node);
  case ND_SEQUENCE:
    return eval_sequence(node);
  case ND_PROCCALL:
    return handle_proc_call(node);
  case ND_LAMBDA:
    return handle_lambda(node, NULL);
  case ND_SET:
    return handle_set(node);
  case ND_DEFINE:
    return handle_definition(node);
  case ND_TOPLEVEL:;
    ASTNode *arg = node->args;
    Obj *res = NULL;
    while (arg) {
      res = eval_ast(arg);
      RETURN_IF_ERROR()
      arg = arg->next;
    }
    return res;
  }
  return NULL;
}