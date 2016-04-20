/*
 * error_manager_test.c
 *
 *  Created on: Apr 20, 2016
 *      Author: duping
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "../../common/common.h"
#include "../../common/error_manager.h"
#include "../../port/atomic.h"
#include "../../port/thread.h"

#include "../common/test_common.h"

typedef struct test_arg TestArg;

struct test_arg
{
  int thread_no;
  int error;
};

static void *test_func(void *);

void *test_func(void *_arg)
{
  TestArg *arg = (TestArg*)_arg;
  Thread *thread_p = getThread();

  assert(arg != NULL && thread_p != NULL);

  for (int i = 0; i < 200; ++i) {

    SET_ERROR(arg->error);

  }

  clearErrors();

  assert(thread_p->err_list_p == NULL);

  return 0;
}

int main(void)
{
  TestArg *arg_arr[TEST_THREAD_COUNT];
  TestArg test_arg_arr[TEST_THREAD_COUNT];

  initErrorManagerEnv();

  // init
  for (int i = 0; i < TEST_THREAD_COUNT; ++i) {
    test_arg_arr[i].thread_no = i;
    test_arg_arr[i].error = ER_GENERIC - i;

    arg_arr[i] = &test_arg_arr[i];
  }

  // test
  create_threads_and_run(TEST_THREAD_COUNT, test_func, (void **)arg_arr);

  printf("error_manager_test SUCCESSES %d!\n", getErrorListBlockCount());

  destroyErrorManagerEnv();

  return 0;
}
