/*
 * base_dequeue_test.c
 *
 *  Created on: May 13, 2016
 *      Author: duping
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../common/common.h"
#include "../../base/base_dequeue.h"

#include "../common/test_common.h"

typedef struct test_arg TestArg;

struct test_arg
{
  int thread_no;
  DeQueue *dq_p;
};

static void *test_func(void *);

void *test_func(void *_arg)
{
  TestArg *arg = (TestArg*)_arg;
  DeQueueValue value;

  assert(arg != NULL);

  for (int i = 0; i < 1000; ++i) {
    value.i = i;
    switch (arg->thread_no % 4) {
    case 0:
      deQueuePush(arg->dq_p, value);
      break;
    case 1:
      deQueuePop(arg->dq_p, &value);
      break;
    case 2:
      deQueuePushFront(arg->dq_p, value);
      break;
    case 3:
      deQueuePopFront(arg->dq_p, &value);
      break;
    }
  }

  //printf("Thread %d: the String value is %s\n", arg->thread_no, arg->s_p->bytes);

  return 0;
}

int main(void) {
  int error = NO_ERROR;
  int i;
  DeQueue *dq_p = createDeQueue(20);
  DeQueueValue value;
  bool succeed = true;

  for (i = 0; i < 1000; ++i) {
    value.i = i;

    if ((i & 0x01) == 0) {
      error = deQueuePush(dq_p, value);
      if (error != NO_ERROR) {
        return error;
      }
    } else {
      error = deQueuePushFront(dq_p, value);
      if (error != NO_ERROR) {
        return error;
      }
    }
  }

  i = 0;
  while (!isDeQueueEmpty(dq_p)) {
    if ((i & 0x01) == 1) {
      error = deQueuePop(dq_p, &value);
      if (error != NO_ERROR) {
        break;
      }
    } else {
      error = deQueuePopFront(dq_p, &value);
      if (error != NO_ERROR) {
        break;
      }
    }

    ++i;
    if (i + value.i != 1000) {
      succeed = false;
      break;
    }
//    printf("%d ", value.i);
//    if (++i % 30 == 0) {
//      printf("\n");
//    }
  }

  //---------------------------------------------
  TestArg *arg_arr[TEST_THREAD_COUNT];
  TestArg test_arg_arr[TEST_THREAD_COUNT];

  // init
  for (int i = 0; i < TEST_THREAD_COUNT; ++i) {
    test_arg_arr[i].thread_no = i;
    test_arg_arr[i].dq_p = dq_p;

    arg_arr[i] = &test_arg_arr[i];
  }

  // test
  create_threads_and_run(TEST_THREAD_COUNT, test_func, (void **)arg_arr);

  DeQueueUnit *unit_p = NULL;

  i = 0;
  unit_p = dq_p->free_list_p;
  while (unit_p != NULL) {
    ++i;
    unit_p = unit_p->next_p;
  }

  int j = 0;
  unit_p = dq_p->front_p;
  while (unit_p != NULL) {
    ++j;
    unit_p = unit_p->next_p;
  }

  if (succeed) {
    printf ("base_dequeue_test succeed .. %d, %d.\n", i, j);
  } else {
    printf ("base_dequeue_test failed .. %d, %d.\n", i, j);
  }

  destroyDeQueue(dq_p);

  return 0;
}
