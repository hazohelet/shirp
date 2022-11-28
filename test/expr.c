#include "shirp.h"

Frame *env;
extern bool lexical_error;
extern bool syntax_error;
extern bool eval_error;
extern Token *cur;
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

void test_int(char *str, int64_t expected) {
  Obj *val = eval_str(str);
  bool success = assert_int(val, expected);
  if (success)
    fprintf(stderr, "\x1b[1m\x1b[32mSUCCESS\x1b[0m: `%s` == `%ld`\n", str,
            expected);
  else
    fprintf(
        stderr,
        "\x1b[1m\x1b[31mFAILED\x1b[0m: `%s` evaluates to `%ld`, not = `%ld`\n",
        str, val->num_val.int_val, expected);
}

void finalize() {
  if (failure) {
    fprintf(stderr, "\x1b[1m\x1b[31mTEST FAILED\x1b[0m\n");
    return;
  }
  fprintf(stderr, "\x1b[1m\x1b[32mTEST SUCCESS\x1b[0m\n");
}
int main() {
  env = push_new_frame(NULL);
  test_int("42", 42);
  test_int("(+ 1 2)", 3);
  test_int("(+ (- (* (* 1 2) 3) (* 4 5)) (+ (* 6 7) (* 8 9)))", 100);
  eval_str("(define a 100)");
  test_int("a", 100);
  eval_str("(define b (+ 10 1))");
  test_int("b", 11);
  test_int("(if (< a b) 10 20)", 20);
  eval_str("(define f (lambda (x) (+ x 1)))");
  test_int("(f 10)", 11);
  eval_str("(define g (lambda (x y) (+ x y)))");
  test_int("(g 3 5)", 8);
  eval_str("(define h (lambda (x) (lambda (y) (+ x (* a y)))))");
  test_int("((h 9) 9)", 909);
  eval_str("(define i (lambda (x) (if (< x a) a x)))");
  test_int("(i (f 100))", 101);
  test_int("(let ((a 1)) (i 10))", 100);
  test_int("(let ((a 10) (b (f a)) (c (- b 1))) (let ((a 10) (b (f a)) (c (- b "
           "1))) (+ (+ a b) c)))",
           121);
  test_int("a", 100);
  /*
  eval_str("(define l (cons 1 (cons 2 (cons 3 (list)))))");
  test_list("l", {1, 2, 3});
  test_int("(car (cdr l)", 2);
   */
  eval_str("(define fact (lambda (n) (if (< n 1) 1 (* n (fact (- n 1))))))");
  test_int("(fact 10)", 3628800);

  finalize();
  return 0;
}