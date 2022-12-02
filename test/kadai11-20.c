#include "testtools.h"

void println(char *str) { fprintf(stderr, "%s\n", str); }

int main() {
  test_init();
  println("S11");
  eval_and_print("(define (my-reverse lst) (define (reverse-rec lst cur) (if "
                 "(null? lst) cur (reverse-rec (cdr lst) (cons (car lst) "
                 "cur)))) (reverse-rec lst '()))");
  test_list_int("(my-reverse '(1 2 3))", (int64_t[]){3, 2, 1}, 3);

  println("S12");
  eval_and_print(
      "(define alist '((1 . \"One\") (2 . \"Two\") (3 . \"Three\")))");
  eval_and_print("(define (my-assoc key alist) (if (null? alist) #f (if "
                 "(equal? key (car (car "
                 "alist))) (car alist) (my-assoc key (cdr alist)))))");
  test_int("(car (my-assoc 1 alist))", 1);
  test_string("(cdr (my-assoc 1 alist))", "One");
  test_int("(car (my-assoc 2 alist))", 2);
  test_string("(cdr (my-assoc 2 alist))", "Two");
  test_bool("(my-assoc 4 alist)", false);
  eval_and_print("(define alist '((() . \"nully\") ((1 2 3) . \"Three\") ((1 "
                 "2 . 3) . \"CONS\")))");
  test_string("(cdr (my-assoc '() alist))", "nully");
  test_string("(cdr (my-assoc '(1 2 3) alist))", "Three");
  test_bool("(my-assoc '(1 2) alist)", false);
  test_string("(cdr (my-assoc '(1 2 . 3) alist))", "CONS");

  println("S13");
  eval_and_print("(define (my-fold op init l) (if (null? l) init (my-fold op "
                 "(op (car l) init) (cdr l))))");
  eval_and_print("(define (my-reverse lst) (my-fold cons '() lst))");
  test_list_int("(my-reverse '(1 2 3))", (int64_t[]){3, 2, 1}, 3);

  println("S14");
  eval_and_print("(define (complex a b) (cons a b))");
  eval_and_print("(define (complex= c1 c2)"
                 "(and (= (car c1) (car c2))"
                 "(= (cdr c1) (cdr c2))))");
  eval_and_print("(define (complex+ c1 c2)"
                 "(complex (+ (car c1) (car c2)) (+ (cdr c1) (cdr c2))))");
  eval_and_print("(define (complex* c1 c2)"
                 "(define r1 (car c1))"
                 "(define r2 (car c2))"
                 "(define i1 (cdr c1))"
                 "(define i2 (cdr c2))"
                 "(complex (- (* r1 r2) (* i1 i2)) (+ (* r1 i2) (* r2 i1))))");
  test_int("(car (complex+ (complex 1 2) (complex 3 4)))", 4);
  test_int("(cdr (complex+ (complex 1 2) (complex 3 4)))", 6);
  test_int("(car (complex* (complex 4 2) (complex -9 4)))", -44);
  test_int("(cdr (complex* (complex 4 2) (complex -9 4)))", -2);

  /* TODO impl cond blocks
    println("S15");
    eval_and_print("(define (exp1 a n)"
                   "(cond ((< n 0) 0)"
                   "((= n 0) 1)"
                   "((even? n) (* (exp1 a (/ n 2)) (exp1 a (/ n 2))))"
                   "(else (* a (exp1 a (- n 1))))))");
    eval_and_print("(define (exp2 a n)"
                   "(let ((square (lambda (x) (* x x))))"
                   "(cond ((< n 0) 0)"
                   "((= n 0) 1)"
                   "((even? n) (square (exp2 a (/ n 2))))"
                   "(else (* a (exp2 a (- n 1)))))))");
                   */

  println("S17");
  eval_and_print("(define (variable? x) (symbol? x))");
  eval_and_print("(define (same-variable? v1 v2) (and (variable? v1) "
                 "(variable? v2) (eq? v1 v2)))");

  test_finalize();
  return 0;
}