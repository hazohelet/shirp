
#include "testtools.h"

int main() {
  test_init();
  eval_and_print("(define a '+)");
  test_symbol("a", "+");
  test_symbol("'+", "+");
  test_symbol("'+.asdf.", "+.asdf.");
  eval_and_print("(define (a) '+)");
  test_symbol("(a)", "+");
  eval_and_print("(define eval '(+ 1  2 3))");
  test_symbol("(car eval)", "+");
  test_int("(car (cdr eval))", 1);
  test_int("(car (cdr (cdr eval)))", 2);
  test_int("(car (cdr (cdr (cdr eval))))", 3);
  eval_and_print("(define con '(42 53 64 . 18))");
  test_int("(car con)", 42);
  test_int("(car (cdr con))", 53);
  test_int("(car (cdr (cdr con)))", 64);
  test_int("(cdr (cdr (cdr con)))", 18);

  test_finalize();
  return 0;
}