/*
 * atomic_test.c
 *
 *  Created on: Mar 14, 2016
 *      Author: duping
 */

#include <stdio.h>
#include <stdlib.h>

#include "../../common/type.h"
#include "../../port/atomic.h"

#include "../common/test_common.h"

typedef struct test_arg TestArg;

struct test_arg
{
  int thread_no;
  volatile int *vi_p;
  int *i_p;
};

static void *test_func(void *);

void *test_func(void *_arg)
{
  TestArg *arg = (TestArg*)_arg;
  bool plus = false;

  assert(arg != NULL);

  plus = (arg->thread_no%2 == 0);

  for (int i = 0; i < 100; ++i) {
    lock_int(arg->vi_p);

    if (plus) {
      *arg->i_p += 1;
    } else {
      *arg->i_p -= 1;
    }

    //memory_barrier();

    unlock_int(arg->vi_p);
  }

  printf("Thread %d: the value of i is %d.\n", arg->thread_no, *arg->i_p);

  return 0;
}

int main(void)
{
  TestArg *arg_arr[TEST_THREAD_COUNT];
  TestArg test_arg_arr[TEST_THREAD_COUNT];

  int n = 1, start_n;
  volatile int vi = 0;

  start_n = n;
  printf("Start ----- the value of i is %d.\n",  start_n);

  // init
  for (int i = 0; i < TEST_THREAD_COUNT; ++i) {
    test_arg_arr[i].thread_no = i;
    test_arg_arr[i].i_p = &n;
    test_arg_arr[i].vi_p = &vi;

    arg_arr[i] = &test_arg_arr[i];
  }

  // test
  create_threads_and_run(TEST_THREAD_COUNT, test_func, (void **)arg_arr);

  printf("end ----- the value of i is %d.\n",  n);

  if (start_n != n) {
    printf("atomic_test FAILS!\n");
  } else {
    printf("atomic_test SUCCESSES!\n");
  }

  return 0;
}
