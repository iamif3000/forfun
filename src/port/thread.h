/*
 * thread.h
 *
 *  Created on: Apr 18, 2016
 *      Author: duping
 */

#include <pthread.h>

#include "../common/error_manager.h"

#ifndef THREAD_H_
#define THREAD_H_

typedef struct os_thread Thread;

struct os_thread {
  pthread_t tid;
  pthread_cond_t cond;
  ErrorList *err_list_p;
};

Thread *getThread();

#endif /* THREAD_H_ */
