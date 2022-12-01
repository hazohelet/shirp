#include "testtools.h"

void println(char *str) { fprintf(stderr, "%s\n", str); }

int main() {
  test_init();
  println("S1");
  eval_and_print("(define min-of-two (lambda (x y) (if (< x y) x y)))");
  eval_and_print("(define min-of-four (lambda (a b c d) (min-of-two "
                 "(min-of-two a b) (min-of-two c d))))");
  test_int("(min-of-four 3 5 4 6)", 3);

  println("S2");
  eval_and_print("(define (odd? n) (if (= (remainder n 2) 1) #t #f))");
  eval_and_print(
      "(define (count-odd x saved) (if (odd? x) (+ saved 1) saved))");
  eval_and_print("(define (odds a b c d e) (count-odd e (count-odd d "
                 "(count-odd c (count-odd b "
                 "(count-odd a 0))))))");
  eval_and_print(
      "(define (even<odd? a b c d e) (if (< (odds a b c d e) 3) #f #t))");
  test_bool("(even<odd? 1 2 3 4 5)", true);
  test_bool("(even<odd? 7 1 2 4 9)", true);

  println("S3");
  eval_and_print(
      "(define (my-gcd a b) (if (= b 0) a (my-gcd b (remainder a b))))");
  test_int("(my-gcd 9801 1287)", 99);

  println("S4");
  eval_and_print(
      "(define (fib n) (if (< n 2) n (+ (fib (- n 1)) (fib (- n 2)))))");
  test_int("(fib 11)", 89);

  println("S5");
  eval_and_print(
      "(define (plus-naive a b) (if (<= a 0) b (+ (plus-naive (- a 1) b) 1)))");
  eval_and_print("(define (plus-tail-rec a b) (if (<= a 0) b (plus-tail-rec (- "
                 "a 1) (+ b 1))))");
  test_int("(plus-naive 3 6)", 9);
  test_int("(plus-tail-rec 7 4)", 11);

  println("S6");
  eval_and_print(
      "(define (fib-tail a b n) (if (= n 0) a (fib-tail b (+ a b ) (- n 1))))");
  eval_and_print("(define (fib2 n) (fib-tail 0 1 n))");
  test_int("(fib2 44)", 701408733);
  GC_mark_and_sweep();

  println("S7");
  eval_and_print("(define (abs n) (if (< n 0) (- 0 n) n))");
  eval_and_print(
      "(define (deriv f dx) (lambda (x) (/ (- (f (+ x dx)) (f x)) dx)))");
  eval_and_print("(define (sqrt-base x) (lambda (t) (- (square t) x)))");
  eval_and_print(
      "(define (good-enough? g guess) (< (abs (g guess)) 0.0000001))");
  eval_and_print(
      "(define (improve g d guess) (- guess (/ (g guess) (d guess))))");
  eval_and_print(
      "(define (newton-iter2 g guess) (define d (deriv g 0.0001)) (if "
      "(good-enough? g guess) guess (newton-iter2 g (improve g d guess))))");
  eval_and_print("(define (square x) (* x x))");
  test_float("((deriv square 0.0001) 3)", 6.000100000012054);
  eval_and_print("(define (sqrt3 x) (newton-iter2 (sqrt-base x) 1.0))");
  test_float("(sqrt3 2)", 1.4142135624530596);
  GC_mark_and_sweep();

  println("S8");
  eval_and_print("(define (compose f g) (lambda (x) (g (f x))))");
  test_int("((compose (lambda (x) (+ x 1)) (lambda (x) (* x x))) 2)", 9);
  GC_mark_and_sweep();

  println("S9");
  eval_and_print("(define (f1 lst) (car (car (cdr (cdr lst)))))");
  eval_and_print("(define (f2 lst) (car (car lst)))");
  eval_and_print("(define (f3 lst) (car (cdr (car (cdr (car (cdr (car (cdr "
                 "(car (cdr (car (cdr lst)))))))))))))");
  test_int("(f1 '(1 2 (3 4) 5))", 3);
  test_int("(f2 '((3)))", 3);
  test_int("(f3 '(1 (2 (4 (5 (6 (7 3)))))))", 3);
  eval_and_print("(define (sum-total lst) (if (null? lst) 0 (+ (car lst) "
                 "(sum-total (cdr lst)))))");
  test_int("(sum-total (list 1 2 3 5))", 11);

  test_finalize();
  return 0;
}