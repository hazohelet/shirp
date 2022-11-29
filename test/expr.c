#include "shirp.h"

Frame *env;
extern bool lexical_error;
extern bool syntax_error;
extern bool eval_error;
extern Token *cur;
extern Obj *nillist;
bool failure = false;

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
}

void finalize() {
  if (failure) {
    fprintf(stderr, "\x1b[1m\x1b[31mTEST FAILED\x1b[0m\n");
    return;
  }
  fprintf(stderr, "\x1b[1m\x1b[32mTEST SUCCESS\x1b[0m\n");
}

void eval_and_print(char *str) {
  fprintf(stderr, "\x1b[33mRUNNING\x1b[0m: `%s`\n", str);
  eval_str(str);
}

int main() {
  env = push_new_frame(NULL);
  test_int("42", 42);
  test_int("(+ 1 2)", 3);
  test_float("(+ 4.2 0)", 4.2);
  test_float("(+ 4.2 2.2)", 6.4);
  test_float("(+ 4 2.2)", 6.2);
  test_float("(+ 4.4 2)", 6.4);
  test_int("(+ (- (* (* 1 2) 3) (* 4 5)) (+ (* 6 7) (* 8 9)))", 100);
  eval_and_print("(define a 100)");
  test_int("a", 100);
  eval_and_print("(define b (+ 10 1))");
  test_int("b", 11);
  test_int("(if (< a b) 10 20)", 20);
  eval_and_print("(define f (lambda (x) (+ x 1)))");
  test_int("(f 10)", 11);
  eval_and_print("(define g (lambda (x y) (+ x y)))");
  test_int("(g 3 5)", 8);
  eval_and_print("(define h (lambda (x) (lambda (y) (+ x (* a y)))))");
  test_int("((h 9) 9)", 909);
  eval_and_print("(define i (lambda (x) (if (< x a) a x)))");
  test_int("(i (f 100))", 101);
  test_int("(let ((a 1)) (i 10))", 100);
  test_int("(let ((a 10) (b (f a)) (c (- b 1))) (let ((a 10) (b (f a)) (c (- b "
           "1))) (+ (+ a b) c)))",
           121);
  test_int("a", 100);
  eval_and_print("(define l (cons 1 (cons 2 (cons 3 (list)))))");
  int64_t expected[] = {1, 2, 3};
  test_list_int("l", expected, 3);
  test_int("(car (cdr l))", 2);
  eval_and_print(
      "(define fact (lambda (n) (if (< n 1) 1 (* n (fact (- n 1))))))");
  test_int("(fact 10)", 3628800);
  eval_and_print(
      "(define gcd (lambda (a b) (if (= b 0) a (gcd b (remainder a b)))))");
  test_int("(gcd 9801 1287)", 99);
  /* TODO: impl Garbage Collection
  test_int("(gcd 1287 9801)", 99);
  */

  finalize();
  return 0;
}