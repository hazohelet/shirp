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

  println("S15");
  eval_and_print("(define (even? n) (= (remainder n 2) 0))");
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
  test_int("(exp1 2 10)", 1024);
  test_int("(exp2 2 10)", 1024);
  test_int("(exp1 5 5)", 3125);
  test_int("(exp2 5 5)", 3125);

  println("S17");
  eval_and_print("(define (variable? x) (symbol? x))");
  eval_and_print("(define (same-variable? v1 v2) (and (variable? v1) "
                 "(variable? v2) (eq? v1 v2)))");
  eval_and_print("(define (make-sum a1 a2)"
                 "(cond ((and (number? a1) (eq? a1 0)) a2)"
                 "((and (number? a2) (eq? a2 0)) a1)"
                 "((and (number? a1) (number? a2)) (+ a1 a2))"
                 "(else (list '+ a1 a2))))");
  eval_and_print("(define (sum? x) (and (pair? x) (eq? (car x) '+)))");
  eval_and_print("(define (make-product a1 a2)"
                 "(cond ((and (number? a1) (eq? a1 0)) 0)"
                 "((and (number? a2) (eq? a2 0)) 0)"
                 "((and (number? a1) (eq? a1 1)) a2)"
                 "((and (number? a2) (eq? a2 1)) a1)"
                 "((and (number? a1) (number? a2)) (* a1 a2))"
                 "(else (list '* a1 a2))))");
  eval_and_print("(define (product? x) (and (pair? x) (eq? (car x) '*)))");
  eval_and_print("(define (addend s) (car (cdr s)))");
  eval_and_print("(define (augend s) (car (cdr (cdr s))))");
  eval_and_print("(define (deriv exp var)"
                 "(cond ((number? exp) 0)"
                 "((variable? exp) (if (same-variable? exp var) 1 0))"
                 "((sum? exp) (make-sum (deriv (addend exp) var)"
                 "(deriv (augend exp) var)))"
                 "((product? exp) (make-sum (make-product (deriv (addend exp) "
                 "var) (augend exp))"
                 "(make-product (deriv (augend exp) var) (addend exp))))))");
  test_symbol("(deriv '(* x y) 'x)", "y");
  test_symbol("(deriv '(+ 5 (* y (+ x z))) 'x)", "y");

  println("S18");
  eval_and_print("(define a 100)");
  eval_and_print("(define b 1)");
  test_int("(let ((a 10) (b (+ a 1)) (c (- b 1))) (let ((a c) (b (+ a 1)) (c "
           "(let ((b 9)) (- b a)))) (+ a b c)))",
           10);

  println("S19");
  eval_and_print("(define a 100)");
  eval_and_print("(define h (lambda (x) (lambda (y) (+ x (* a y)))))");
  test_int("(let ((a 0)) ((h 1) (+ a 5)))", 501);

  println("S20");
  eval_and_print("(define (make-monitored f)"
                 "(let ((num 0))"
                 "(lambda (x)"
                 "(cond ((eq? x 'how-many-calls?) num)"
                 "((eq? x 'reset-count) (set! num 0))"
                 "(else (set! num (+ num 1)) (f x))))))");
  eval_and_print("(define s (make-monitored sqrt))");
  test_float("(s 100)", 10);
  test_int("(s 'how-many-calls?)", 1);
  test_float("(s 400)", 20);
  test_int("(s 'how-many-calls?)", 2);
  eval_and_print("(s 'reset-count)");
  test_float("(s 900)", 30);
  test_int("(s 'how-many-calls?)", 1);

  test_finalize();
  return 0;
}