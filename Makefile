CFLAGS := -Wall -Wextra -Wconversion -I.
LIBS := -lm
ODIR := objectfiles
SHIRP_SRC := *.c
TEST_OBJ := *.o

shirp: *.c *.h prelude.scm
	$(CC) -O3 $(CFLAGS) --undefine-macro DEBUG *.c $(LIBS) -o shirp

debug: *.c *.h prelude.scm
	$(CC) -O3 $(CFLAGS) -DDEBUG *.c $(LIBS) -o shirp_debug

test: test_expr test_kadai1 test_kadai2 test_quote prelude.scm
	./test_expr && ./test_kadai1 && ./test_kadai2 && ./test_quote


	@if [ "$$?" = "0" ]; then\
		printf "\033[32mAll TESTS PASSED\033[0m\n";\
	else\
		printf "\033[1m\033[31mAll TESTS PASSED\033[0m\n";\
	fi;

test_expr: test/expr.c testtools prelude.scm
	$(CC) -O3 $(CFLAGS )$(TEST_OBJ) test/expr.c -I. $(LIBS) -o test_expr

test_kadai1: test/kadai1.c testtools prelude.scm
	$(CC) -O3 $(CFLAGS )$(TEST_OBJ) test/kadai1.c -I. $(LIBS) -o test_kadai1

test_kadai2: test/kadai2.c testtools prelude.scm
	$(CC) -O3 $(CFLAGS )$(TEST_OBJ) test/kadai2.c -I. $(LIBS) -o test_kadai2

test_quote: test/quote.c testtools prelude.scm
	$(CC) -O3 $(CFLAGS )$(TEST_OBJ) test/quote.c -I. $(LIBS) -o test_quote

testtools: test/testtools.c test/testtools.h *.c
	$(CC) -O3 $(CFLAGS) -DTEST  -I. -lm -c *.c test/testtools.c

clean:
	rm -f shirp shirp_debug test_expr test_kadai1 test_kadai2 test_quote *.o