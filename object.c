#include "shirp.h"

void print_obj(Obj *obj) {
  if (obj->typ == INT_TY)
    fprintf(stderr, "%ld\n", obj->num_val.int_val);
  else if (obj->typ == FLOAT_TY)
    fprintf(stderr, "%lf\n", obj->num_val.float_val);
  else
    fprintf(stderr, "Eval Error");
}

Obj *new_obj(ObjType typ) {
  Obj *obj = (Obj *)shirp_malloc(sizeof(Obj));
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

Obj *eval_ast(ASTNode *node) {
  switch (node->kind) {
  case ND_IDENT:
    debug_log("To be implemented");
    break;
  case ND_NUMBER:
    return node->tok->obj;
  case ND_PROCCALL:
    return handle_proc_call(node);
  case ND_LAMBDA:
    debug_log("To be implemented");
    break;
  case ND_DEFINE:
    debug_log("To be implemented");
    break;
  }
  return NULL;
}