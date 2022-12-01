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
  eval_and_print(
      "(define (my-assoc key alist) (if (null? alist) #f (if (= key (car (car "
      "alist))) (car alist) (my-assoc key (cdr alist)))))");
  test_int("(car (my-assoc 1 alist))", 1);
  test_string("(cdr (my-assoc 1 alist))", "One");
  test_int("(car (my-assoc 2 alist))", 2);
  test_string("(cdr (my-assoc 2 alist))", "Two");
  test_bool("(my-assoc 4 alist)", false);

  println("S13");
  eval_and_print("(define (my-fold op init l) (if (null? l) init (my-fold op "
                 "(op (car l) init) (cdr l))))");
  eval_and_print("(define (my-reverse lst) (my-fold cons '() lst))");
  test_list_int("(my-reverse '(1 2 3))", (int64_t[]){3, 2, 1}, 3);

  println("S14");
  eval_and_print("(define (complex a b) (cons a b))");

  test_finalize();
  return 0;
}