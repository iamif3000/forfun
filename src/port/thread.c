/*
 * thread.c
 *
 *  Created on: Apr 18, 2016
 *      Author: duping
 */

#include <unistd.h>
#include <stdlib.h>

#include "../common/common.h"

#include "thread.h"

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void makeKey();
static void destroyThread(void *p);

/*
 * static
 */
void makeKey()
{
  pthread_key_create(&key, destroyThread);
}

/*
 * static
 */
void destroyThread(void *p)
{
  Thread *thread_p = (Thread*)p;

  if (thread_p != NULL) {
    (void)pthread_cond_destroy(&thread_p->cond);
    free(thread_p);
  }
}

/*
 * NOTE : the checking for pthread_xxx return value is ignored,
 *        if we get a NULL return value, we should report an error
 */
Thread *getThread()
{
  int error = 0;
  Thread *thread_p = NULL;

  (void)pthread_once(&key_once, makeKey);

  thread_p = (Thread*)pthread_getspecific(key);
  if (thread_p == NULL) {
    thread_p = (Thread*)malloc(sizeof(Thread));
    if (thread_p != NULL) {
      thread_p->tid = pthread_self();
      thread_p->err_list_p = NULL;

      error = pthread_cond_init(&thread_p->cond, NULL);
      if (error == 0) {
        error = pthread_setspecific(key, thread_p);
        if (error < 0) {
          destroyThread(thread_p);
          thread_p = NULL;
        }
      } else {
        free(thread_p);
        thread_p = NULL;
      }
    }
  }

  return thread_p;
}
