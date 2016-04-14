/*
 * base_string_test.c
 *
 *  Created on: Mar 14, 2016
 *      Author: duping
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "../../common/type.h"
#include "../../port/atomic.h"
#include "../../base/base_string.h"

#include "../common/test_common.h"

typedef struct test_arg TestArg;

struct test_arg
{
  int thread_no;
  String *s_p;
};

static void *test_func(void *);

void *test_func(void *_arg)
{
  TestArg *arg = (TestArg*)_arg;

  assert(arg != NULL);

  for (int i = 0; i < 100; ++i) {

    arg->s_p = editString(arg->s_p);

    assert(arg->s_p != NULL && arg->s_p->bytes != NULL);

    arg->s_p->bytes[0] = 'A' + (i + arg->thread_no) % 26;

    printf("Thread %d: the String value is %s\n", arg->thread_no, arg->s_p->bytes);
  }

  //printf("Thread %d: the String value is %s\n", arg->thread_no, arg->s_p->bytes);

  freeString(arg->s_p);

  return 0;
}

int main(void)
{
  TestArg *arg_arr[TEST_THREAD_COUNT];
  TestArg test_arg_arr[TEST_THREAD_COUNT];

  const char cstr[] = "String test.";
  String *str = allocString(sizeof(cstr), cstr);

  // init
  for (int i = 0; i < TEST_THREAD_COUNT; ++i) {
    test_arg_arr[i].thread_no = i;
    test_arg_arr[i].s_p = refString(str);

    arg_arr[i] = &test_arg_arr[i];
  }

  // test
  create_threads_and_run(TEST_THREAD_COUNT, test_func, (void **)arg_arr);

  if (strncmp(cstr, str->bytes, sizeof(cstr)) != 0) {
    printf("base_string FAILS!\n");
  } else {
    printf("base_string SUCCESSES!\n");
  }

  freeString(str);

  return 0;
}
