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

  test_finalize();
  return 0;
}