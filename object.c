#include "shirp.h"

extern Frame *env;
bool eval_error = false;

void print_obj(Obj *obj) {
  switch (obj->typ) {
  case UNDEF_TY:
    fprintf(stderr, "#<undef>\n");
    break;
  case BOOL_TY:
    fprintf(stderr, "%s\n", obj->num_val.bool_val ? "#t" : "#f");
    break;
  case INT_TY:
    fprintf(stderr, "%ld\n", obj->num_val.int_val);
    break;
  case FLOAT_TY:
    fprintf(stderr, "%lf\n", obj->num_val.float_val);
    break;
  default:
    fprintf(stderr, "Eval Error");
  }
}

Obj *new_obj(ObjType typ) {
  Obj *obj = (Obj *)shirp_malloc(sizeof(Obj));
  obj->typ = typ;
  return obj;
}

/*
static Obj *create_bool_obj(bool val) {
  Obj *obj = new_obj(BOOL_TY);
  obj->num_val.bool_val = val;
  return obj;
}
*/

Obj *true_obj = &(Obj){BOOL_TY, {true}, NULL, NULL, NULL, NULL, NULL};
Obj *false_obj = &(Obj){BOOL_TY, {false}, NULL, NULL, NULL, NULL, NULL};

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

void convert_to_float(Obj *obj) {
  if (obj->typ == INT_TY) {
    obj->typ = FLOAT_TY;
    obj->num_val.float_val = (double)obj->num_val.int_val;
  }
}

void Obj_add_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->num_val.int_val = op1->num_val.int_val + op2->num_val.int_val;
  } else {
    dst->typ = FLOAT_TY;
    convert_to_float(op1);
    convert_to_float(op2);
    dst->num_val.float_val = op1->num_val.float_val + op2->num_val.float_val;
  }
}

void Obj_sub_operation(Obj *dst, Obj *op1, Obj *op2) {
  if (op1->typ == INT_TY && op2->typ == INT_TY) {
    dst->typ = INT_TY;
    dst->num_val.int_val = op1->num_val.int_val - op2->num_val.int_val;
  } else {
    dst->typ = FLOAT_TY;
    convert_to_float(op1);
    convert_to_float(op2);
    dst->num_val.float_val = op1->num_val.float_val - op2->num_val.float_val;
  }
}

Obj *handle_proc_call(ASTNode *node) {
  if (match_tok(node->tok, "+")) {
    debug_log("+ Handled!");
    Obj *result = new_int_obj(0);
    ASTNode *arg = node->args;
    while (arg) {
      Obj_add_operation(result, result, eval_ast(arg));
      arg = arg->next;
    }
    return result;
  } else if (match_tok(node->tok, "-")) {
    debug_log("- Handled!");
    Obj *result = eval_ast(node->args);
    ASTNode *arg = node->args->next;
    while (arg) {
      Obj_sub_operation(result, result, eval_ast(arg));
      arg = arg->next;
    }
    return result;
  }
  return NULL;
}

static bool is_not_false(Obj *obj) {
  return !(obj->typ == BOOL_TY && obj->num_val.bool_val == false);
}

Obj *eval_if(ASTNode *node) {
  ASTNode *test = node->args;
  Obj *test_val = eval_ast(test);
  if (eval_error)
    return NULL;
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

Obj *eval_ast(ASTNode *node) {
  if (!node) {
    return new_obj(UNDEF_TY);
  }
  switch (node->kind) {
  case ND_IDENT:
    debug_log("handle identifier in eval, env: %p, outer: %p", env, env->outer);
    debug_log("key: %.*s", node->tok->len, node->tok->loc);
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
    debug_log("To be implemented");
    break;
  case ND_DEFINE:
    handle_definition(node);
    break;
  }
  return NULL;
}