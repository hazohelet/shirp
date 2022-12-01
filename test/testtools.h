#ifndef TESTTOOLS_H
#define TESTTOOLS_H
#include "shirp.h"

void test_init();
void test_int(char *str, int64_t expected);
void test_float(char *str, double expected);
void test_bool(char *str, bool expected);
void test_list_int(char *str, int64_t expected[], size_t size);
void test_symbol(char *str, char *expected);
void test_string(char *str, char *expected);
void test_finalize();
void eval_and_print(char *str);
#endif