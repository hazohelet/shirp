#include "testtools.h"

Frame *env;
extern bool lexical_error;
extern bool syntax_error;
extern bool eval_error;
extern Token *cur;
extern Obj *nillist;
bool failure = false;

void test_init() {
  env = push_new_frame(NULL);
  GC_init();
}

Obj *eval_str(char *str) {
  Token head = {};
  tokenize(str, &head);
  if (lexical_error) {
    failure = true;
    return NULL;
  }
  cur = head.next;
  ASTNode *node = program(&head);
  if (syntax_error) {
    failure = true;
    return NULL;
  }
  Obj *val = eval_ast(node);
  if (eval_error) {
    failure = true;
    return NULL;
  }
  return val;
}

bool assert_int(Obj *tested, int64_t expected) {
  if (tested->typ != INT_TY) {
    failure = true;
    return false;
  }
  if (tested->num_val.int_val != expected) {
    failure = true;
    return false;
  }
  return true;
}

bool assert_float(Obj *tested, double expected) {
  if (tested->typ != FLOAT_TY) {
    failure = true;
    return false;
  }
  if (tested->num_val.float_val != expected) {
    failure = true;
    return false;
  }
  return true;
}

void test_int(char *str, int64_t expected) {
  Obj *val = eval_str(str);
  bool success = assert_int(val, expected);
  if (success)
    fprintf(stderr, "\x1b[1m\x1b[32mSUCCESS\x1b[0m: `%s` == `%" PRId64 "`\n",
            str, expected);
  else
    fprintf(stderr,
            "\x1b[1m\x1b[31mFAILED\x1b[0m: `%s` evaluates to `%" PRId64
            "`, not = `%" PRId64 "`\n",
            str, val->num_val.int_val, expected);
  GC_mark_and_sweep();
}

void test_float(char *str, double expected) {
  Obj *val = eval_str(str);
  bool success = assert_float(val, expected);
  if (success)
    fprintf(stderr, "\x1b[1m\x1b[32mSUCCESS\x1b[0m: `%s` == `%f`\n", str,
            expected);
  else
    fprintf(
        stderr,
        "\x1b[1m\x1b[31mFAILED\x1b[0m: `%s` evaluates to `%lf`, not = `%lf`\n",
        str, val->num_val.float_val, expected);
  GC_mark_and_sweep();
}

void test_list_int(char *str, int64_t expected[], size_t size) {
  Obj *list = eval_str(str);
  if (!is_list(list)) {
    fprintf(stderr,
            "\x1b[1m\x1b[31mFAILED\x1b[0m: `%s` does not evaluate to list\n",
            str);
    failure = true;
    return;
  }
  size_t argc = 0;
  Obj *cell = list;
  while (cell && cell != nillist) {
    argc++;
    cell = cell->cdr;
  }
  if (size != argc) {
    fprintf(stderr,
            "\x1b[1m\x1b[31mFAILED\x1b[0m: `%s` evaluates to list of size "
            "`%ld`, not `%ld`\n",
            str, argc, size);
    failure = true;
    return;
  }
  for (size_t idx = 1; idx <= size; idx++) {
    Obj *car = list->car;
    if (!assert_int(car, expected[idx - 1])) {
      fprintf(stderr,
              "\x1b[1m\x1b[31mFAILED\x1b[0m: `%s` evaluates to list with "
              "%ldth element `%" PRId64 "`, not = `%" PRId64 "`\n",
              str, idx, car->num_val.int_val, expected[idx - 1]);
      failure = true;
      return;
    }
    list = list->cdr;
  }
  fprintf(stderr, "\x1b[1m\x1b[32mSUCCESS\x1b[0m: `%s` == (", str);
  for (size_t i = 0; i < size; i++) {
    fprintf(stderr, "%" PRId64, expected[i]);
    if (i != size - 1)
      fprintf(stderr, " ");
  }
  fprintf(stderr, ")\n");
  GC_mark_and_sweep();
}

bool assert_bool(Obj *tested, bool expected) {
  if (tested->typ != BOOL_TY) {
    failure = true;
    return false;
  }
  if (tested->num_val.bool_val != expected) {
    failure = true;
    return false;
  }
  return true;
}

void test_bool(char *str, bool expected) {
  Obj *val = eval_str(str);
  bool success = assert_bool(val, expected);
  if (success)
    fprintf(stderr, "\x1b[1m\x1b[32mSUCCESS\x1b[0m: `%s` == `%s`\n", str,
            expected ? "#t" : "#f");
  else
    fprintf(stderr, "\x1b[1m\x1b[31mFAILED\x1b[0m: `%s` != `%s`\n", str,
            expected ? "#t" : "#f");
  GC_mark_and_sweep();
}

bool assert_symbol(Obj *tested, char *expected) {
  if (tested->typ != SYMBOL_TY) {
    failure = true;
    return false;
  }
  if (strlen(expected) != tested->str_len ||
      strncmp(tested->str_val, expected, tested->str_len) != 0) {
    failure = true;
    return false;
  }
  return true;
}

void test_symbol(char *str, char *expected) {
  Obj *val = eval_str(str);
  bool success = assert_symbol(val, expected);
  if (success)
    fprintf(stderr, "\x1b[1m\x1b[32mSUCCESS\x1b[0m: `%s` == `%s`\n", str,
            expected);
  else
    fprintf(stderr, "\x1b[1m\x1b[31mFAILED\x1b[0m: `%s` != `%s`\n", str,
            expected);
  GC_mark_and_sweep();
}

void test_finalize() {
  if (failure) {
    fprintf(stderr, "\x1b[1m\x1b[31mTEST FAILED\x1b[0m\n");
    return;
  }
  fprintf(stderr, "\x1b[1m\x1b[32mTEST SUCCESS\x1b[0m\n");
}

void eval_and_print(char *str) {
  fprintf(stderr, "\x1b[33mRUNNING\x1b[0m: `%s`\n", str);
  eval_str(str);
  GC_mark_and_sweep();
}