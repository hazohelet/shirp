#include "testtools.h"

void println(char *str) { fprintf(stderr, "%s\n", str); }

int main() {
  test_init();
  println("S1");
  eval_and_print("(define min-of-two (lambda (x y) (if (< x y) x y)))");
  eval_and_print("(define min-of-four (lambda (a b c d) (min-of-two "
                 "(min-of-two a b) (min-of-two c d))))");
  test_int("(min-of-four 3 5 4 6)", 3);

  test_finalize();
  return 0;
}