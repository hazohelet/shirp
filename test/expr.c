#include "testtools.h"

extern GC *gc;

int main() {
  test_init();
  test_int("42", 42);
  test_int("(+ 1 2)", 3);
  test_float("(+ 4.2 0)", 4.2);
  test_float("(+ 4.2 2.2)", 6.4);
  test_float("(+ 4 2.2)", 6.2);
  test_float("(+ 4.4 2)", 6.4);
  test_bool("#f", false);
  test_bool("#t", true);
  test_bool("(< 4.4 2)", false);
  test_bool("(<= 2 2)", true);
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
      "(define fact (lambda (n) (if (< n 1) 1 (* (fact (- n 1)) n))))");
  test_int("(fact 10)", 3628800);
  eval_and_print(
      "(define gcd (lambda (a b) (if (= b 0) a (gcd b (remainder a b)))))");
  test_int("(gcd 9801 1287)", 99);
  test_int("(gcd 1287 9801)", 99);
  GC_collect();
  test_int("(cond (else 42))", 42);
  test_int("(cond (33) (else 42))", 33);
  test_int("(cond (#f) (else 42))", 42);
  test_int("(cond (#f 34) (else 42))", 42);
  test_bool("(= 42 42 42 42 42)", true);
  test_bool("(= 42 42 24 42 42)", false);
  test_bool("(< -12 -3 -2 0 12 44)", true);
  test_bool("(< -12 -3 8 3)", false);
  test_bool("(<= 42 42 42 42 42)", true);
  test_bool("(<= 42 42 43 45 45)", true);
  test_int("(+)", 0);
  test_int("(*)", 1);
  test_int("(- 42)", -42);

  test_finalize();
  return 0;
}