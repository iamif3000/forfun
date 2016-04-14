/*
 * resource_queue_test.c
 *
 *  Created on: Apr 6, 2016
 *      Author: duping
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../../common/common.h"
#include "../../port/atomic.h"
#include "../../port/resource_queue.h"

#include "../common/test_common.h"

typedef struct test_arg TestArg;

struct test_arg
{
  int thread_no;
  ResourceQueue *rq_p;
};

static int rq_alloc_count = 0;

static void *test_func(void *);
static void *test_func2(void *);

void *test_func(void *_arg)
{
  TestArg *arg = (TestArg*)_arg;
  int what = 0, *value = NULL, last = 0;
  ResourceQueue *rq_p = NULL;
  int error = NO_ERROR;

  assert(arg != NULL);

  rq_p = arg->rq_p;

  what = arg->thread_no%3;

  for (int i = 0; i < 100; ++i) {
    switch (what) {
    case 0:
      error = requestResource(rq_p, RQ_REQUEST_READ, (void**)&value);
      if (error != NO_ERROR) {
        printf("Thread %d: request %d fail.\n", arg->thread_no, RQ_REQUEST_READ);
        break;
      }

      assert(value != NULL);

      last = *value;

      (void)releaseResource(rq_p);

      break;
    case 1:
      error = requestResource(rq_p, RQ_REQUEST_WRITE, (void**)&value);
      if (error != NO_ERROR) {
        printf("Thread %d: request %d for -- fail.\n", arg->thread_no, RQ_REQUEST_WRITE);
        break;
      }

      assert(value != NULL);

      last = --*value;

      (void)releaseResource(rq_p);

      break;
    case 2:
      error = requestResource(rq_p, RQ_REQUEST_WRITE, (void**)&value);
      if (error != NO_ERROR) {
        printf("Thread %d: request %d for ++ fail.\n", arg->thread_no, RQ_REQUEST_WRITE);
        break;
      }

      assert(value != NULL);

      last = ++*value;

      (void)releaseResource(rq_p);

      break;
    default:
      break;
    }

    //printf("--Thread %d: the value of i is %d.\n", arg->thread_no, last);
  }

  printf("Thread %d: the value of i is %d.\n", arg->thread_no, last);

  return 0;
}

void *test_func2(void *_arg)
{
  int array_count = 500;

  TestArg *arg = (TestArg*)_arg;
  ResourceQueue *rq_p[array_count];
  int error = NO_ERROR;

  assert(arg != NULL);

  memset(rq_p, 0, sizeof(rq_p));

  for (int i = 0; i < array_count; ++i) {
    rq_p[i] = createResourceQueue(&i, "test2");
    if (rq_p[i] == NULL) {
      error = -1;
      goto end;
    }

    atomic_add(&rq_alloc_count, 1);

    //destroyResourceQueueAndSetNull(rq_p);
  }

end:

  for (int i = 0; i < array_count; ++i) {
    if (rq_p[i] != NULL) {
      destroyResourceQueueAndSetNull(rq_p[i]);
    }
  }

  if (error != NO_ERROR) {
    arg->rq_p = (ResourceQueue*)(uint_ptr)-1;
  }

  printf("Test2 %u end.\n",  arg->thread_no);

  return 0;
}

int main(void)
{
  TestArg *arg_arr[TEST_THREAD_COUNT];
  TestArg test_arg_arr[TEST_THREAD_COUNT];

  int n = 1, start_n;

  int error = NO_ERROR;
  ResourceQueue *rq_p = NULL;

  error = initResourceQueueEnv();
  if (error != NO_ERROR) {
    goto end;
  }

  // test1 : test single queue functionalities
  rq_p = createResourceQueue(&n, "integer queue test");
  if (rq_p == NULL) {
    error = -1;
    goto end;
  }

  start_n = n;
  printf("Start ----- the value of i is %d.\n",  start_n);

  // init
  for (int i = 0; i < TEST_THREAD_COUNT; ++i) {
    test_arg_arr[i].thread_no = i;
    test_arg_arr[i].rq_p = rq_p;

    arg_arr[i] = &test_arg_arr[i];
  }

  // test
  create_threads_and_run(TEST_THREAD_COUNT, test_func, (void **)arg_arr);

  printf("end ----- the value of i is %d.\n",  n);

  if (start_n != n) {
    printf("resource_queue_test1 FAILS! %d, %d\n", start_n, n);
    error = -1;

    goto end;
  } else {
    printf("resource_queue_test1 SUCCESSES! %d, %d\n", start_n, n);
  }

  if (rq_p != NULL) {
    destroyResourceQueueAndSetNull(rq_p);
  }


  // test2 : test ResourceQueue alloc functions
  printf("Start test2.\n",  start_n);

  for (int i = 0; i < TEST_THREAD_COUNT; ++i) {
    test_arg_arr[i].thread_no = i;
    test_arg_arr[i].rq_p = NULL;

    arg_arr[i] = &test_arg_arr[i];
  }

  create_threads_and_run(TEST_THREAD_COUNT, test_func2, (void **)arg_arr);

  printf("end test2.\n",  n);

  for (int i = 0; i < TEST_THREAD_COUNT; ++i) {
    if (test_arg_arr[i].rq_p != NULL) {
      printf("resource_queue_test2 FAILS!\n");
      error = -1;

      goto end;
    }
  }

  printf("resource_queue_test2 SUCCESSES!\n");

end:

  if (rq_p != NULL) {
    destroyResourceQueueAndSetNull(rq_p);
  }

  int i = resourceQueueBlockCount();
  int j = holderListBlockCount();
  printf("rq alloc count %d, rq block count %d, hl block count %d...\n", rq_alloc_count, i, j);

  destropyResourceQueueEnv();

  if (error != NO_ERROR) {
    printf("resource_queue_test ---- FAILS!\n");

    return 1;
  }

  printf("resource_queue_test ---- SUCCESSES!\n");

  return 0;
}
