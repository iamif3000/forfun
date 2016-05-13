/*
 * test_common.h
 *
 *  Created on: Mar 14, 2016
 *      Author: duping
 */

#include <pthread.h>
#include <stddef.h>
#include <assert.h>

#ifndef TEST_COMMON_H_
#define TEST_COMMON_H_

#define TEST_THREAD_COUNT 100

typedef void *(*THREAD_FUNC)(void *);

inline static void create_threads_and_run(int threads_count, THREAD_FUNC func, void **args)
{
  pthread_t thread_arr[threads_count];
  int error = 0;

  // create thread
  for (int i = 0; i < threads_count; ++i) {
    error = pthread_create(&thread_arr[i], NULL, func, args[i]);
    if (error != 0) {
      exit(1);
    }
  }

  for (int i = 0; i < threads_count; ++i) {
    pthread_join(thread_arr[i], NULL);
  }
}

#endif /* COMMON_H_ */
