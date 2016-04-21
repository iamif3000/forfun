/*
 * resource_queue.c
 *
 *  Created on: Apr 1, 2016
 *      Author: duping
 */

// TODO : optimize memory management for ResourceQueue

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../common/common.h"

#include "../port/atomic.h"

#include "resource_queue.h"

#define APPEND_TO_CDLL(head_p, node_p) \
  do { \
    node_p->next_p = head_p; \
    node_p->prev_p = head_p->prev_p; \
    head_p->prev_p->next_p = node_p; \
    head_p->prev_p = node_p; \
  } while(0)

#define REMOVE_FROM_CDLL(node_p) \
  do { \
    node_p->prev_p->next_p = node_p->next_p; \
    node_p->next_p->prev_p = node_p->prev_p; \
    node_p->next_p = NULL; \
    node_p->prev_p = NULL; \
  } while(0)

typedef struct resouece_queue_block ResourceQueueBlock;
typedef struct rq_holder_list_block RQHolderListBlock;
typedef struct wait_wake_func_arg WaitWakeFuncArg;

struct resouece_queue_block {
  ResourceQueueBlock *next_p;
  ResourceQueue list[0];
};

struct rq_holder_list_block {
  RQHolderListBlock *next_p;
  RQHolderList list[0];
};

struct wait_wake_func_arg {
  int error;
  pthread_cond_t *cond_p;
  pthread_mutex_t *mutex_p;
  struct timespec *time_s_p;
};

static volatile int rq_list_block_lock = 0;
static ResourceQueueBlock *rq_list_block_p = NULL;
static ResourceQueue *free_rq_list_p = NULL;

static volatile int holder_list_block_lock = 0;
static RQHolderListBlock *holder_list_block_p = NULL;
static RQHolderList *free_holder_list_p = NULL;

static RQThread *getRQThread();
static int initResource(RQResource *r_p);
static int initResourceQueue(ResourceQueue *rq_p);
static int expandHolderList();
static void initSubject(RQSubject *subject_p);
static RQHolderList *allocHolderList();
static void freeHolderList(RQHolderList *list_p);
static void initHolderList(RQHolderList *list_p);

static int expandRqList();
static ResourceQueue *allocResourceQueue();
static void freeResourceQueue(ResourceQueue *rq_p);

inline static bool isHolderListEmpty(ResourceQueue *rq_p);
inline static void appendToHolderList(ResourceQueue *rq_p, RQHolderList *list_p);
inline static void removeFromHolderList(ResourceQueue *rq_p, RQHolderList *list_p);
inline static bool isWaitListEmpty(ResourceQueue *rq_p);
inline static void appendToWaitList(ResourceQueue *rq_p, RQWaitList *list_p);
inline static void removeFromWaitList(ResourceQueue *rq_p, RQWaitList *list_p);

static int wait_func_wrapper(void *arg_p);
static int timed_wait_func_wrapper(void *arg_p);
static int wake_func_wrapper(void *arg_p);

/*
 * static
 */
RQThread *getRQThread()
{
  return getThread();
}

/*
 * static
 */
bool isHolderListEmpty(ResourceQueue *rq_p)
{
  assert(rq_p != NULL);

  return (rq_p->holder_list.next_p == &rq_p->holder_list);
}

/*
 * static
 */
void appendToHolderList(ResourceQueue *rq_p, RQHolderList *list_p)
{
  RQHolderList *head_p = NULL;

  assert(rq_p != NULL && list_p != NULL);

  head_p = &rq_p->holder_list;

  APPEND_TO_CDLL(head_p, list_p);
}

/*
 * static
 */
void removeFromHolderList(ResourceQueue *rq_p, RQHolderList *list_p)
{
  assert(list_p != NULL && list_p != &rq_p->holder_list);

  REMOVE_FROM_CDLL(list_p);
}

/*
 * static
 */
bool isWaitListEmpty(ResourceQueue *rq_p)
{
  assert(rq_p != NULL);

  return (rq_p->wait_list.next_p == &rq_p->wait_list);
}

/*
 * static
 */
void appendToWaitList(ResourceQueue *rq_p, RQWaitList *list_p)
{
  RQWaitList *head_p = NULL;

  assert(rq_p != NULL && list_p != NULL);

  head_p = &rq_p->wait_list;

  APPEND_TO_CDLL(head_p, list_p);
}

/*
 * static
 */
void removeFromWaitList(ResourceQueue *rq_p, RQWaitList *list_p)
{
  assert(list_p != NULL && list_p != &rq_p->wait_list);

  REMOVE_FROM_CDLL(list_p);
}

/*
 * static
 */
int initResource(RQResource *r_p)
{
  int error = NO_ERROR;

  assert(r_p != NULL);

  r_p->resource_p = NULL;
  r_p->current_type = RQ_REQUEST_READ;

  error = pthread_mutex_init(&r_p->mutex, NULL);
  if (error < 0) {
    error = ER_RQ_CREATE_FAIL;
    SET_ERROR(error);
  }

  return error;
}

/*
 * static
 */
int initResourceQueue(ResourceQueue *rq_p)
{
  int error = NO_ERROR;

  assert(rq_p != NULL);

  rq_p->name_p = NULL;
  rq_p->holder_list.next_p = rq_p->holder_list.prev_p = &rq_p->holder_list;
  rq_p->wait_list.next_p = rq_p->wait_list.prev_p = &rq_p->wait_list;

  error = initResource(&rq_p->resource);

  return error;
}

/*
 * static
 */
int expandHolderList()
{
#define ALLOC_COUNT 512

  int error = NO_ERROR;
  RQHolderListBlock *block_p = NULL;
  RQHolderList *list_p = NULL;
  RQHolderList *free_list_p = NULL;

  block_p = (RQHolderListBlock*)malloc(sizeof(RQHolderListBlock) + sizeof(RQHolderList) * ALLOC_COUNT);
  if (block_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  list_p = block_p->list;
  for (int i = 1; i < ALLOC_COUNT; ++i) {
    list_p[i-1].next_p = &list_p[i];

    initSubject(&list_p[i-1].subject_p);
  }

  list_p[ALLOC_COUNT - 1].next_p = NULL;
  initSubject(&list_p[ALLOC_COUNT-1].subject_p);

  block_p->next_p = holder_list_block_p;
  holder_list_block_p = block_p;

  while (true) {
    free_list_p = free_holder_list_p;
    if (MARKED(free_list_p)) {
      continue;
    }

    list_p[ALLOC_COUNT - 1].next_p = free_list_p;

    if (compare_and_swap_bool(&free_holder_list_p, free_list_p, &list_p[0])) {
      break;
    }
  }

end:

  if (error != NO_ERROR && block_p != NULL) {
    free(block_p);
  }

  return error;

#undef ALLOC_COUNT
}

/*
 * static
 */
void initSubject(RQSubject *subject_p)
{
  int error = NO_ERROR;

  assert(subject_p != NULL);

  subject_p->thread = NULL;

  subject_p->wait_func_p = wait_func_wrapper;
  subject_p->timed_wait_func_p = timed_wait_func_wrapper;
  subject_p->wake_func_p = wake_func_wrapper;
}

/*
 * static
 */
RQHolderList *allocHolderList()
{
  int error = NO_ERROR;
  RQHolderList *list_p = NULL;

  while (true) {
    while (true) {
      list_p = free_holder_list_p;
      if (list_p == NULL) {
        break;
      }

      if (MARKED(list_p) || !compare_and_swap_bool(&free_holder_list_p, list_p, MARK(list_p))) {
        continue;
      }

      if (compare_and_swap_bool(&free_holder_list_p, MARK(list_p), list_p->next_p)) {
        break;
      }

      assert(false);
    }

    if (list_p == NULL) {
      lock_int(&holder_list_block_lock);

      if (free_holder_list_p == NULL) {
        error = expandHolderList();
      }

      unlock_int(&holder_list_block_lock);
    } else {
      initHolderList(list_p);
    }

    if (list_p != NULL || error != NO_ERROR) {
      break;
    }
  }

  return list_p;
}

/*
 * static
 */
void freeHolderList(RQHolderList *list_p)
{
  RQHolderList *free_list_p = NULL;

  if (list_p != NULL) {
    list_p->subject_p.thread = NULL;

    while (true) {
      free_list_p = free_holder_list_p;
      if (MARKED(free_list_p)) {
        continue;
      }

      list_p->next_p = free_list_p;

      if (compare_and_swap_bool(&free_holder_list_p, free_list_p, list_p)) {
        break;
      }
    }
  }
}

/*
 * static
 */
void initHolderList(RQHolderList *list_p)
{
  int error = NO_ERROR;

  assert(list_p != NULL);

  list_p->next_p = list_p->prev_p = NULL;

  initSubject(&list_p->subject_p);

  list_p->error = NO_ERROR;
}

/*
 * static
 */
int expandRqList()
{
#define ALLOC_COUNT 512

  int error = NO_ERROR;
  ResourceQueueBlock *rqb_p = NULL;
  ResourceQueue *rq_p = NULL;
  ResourceQueue *free_list_p = NULL;

  rqb_p = malloc(sizeof(ResourceQueueBlock) + sizeof(ResourceQueue) * ALLOC_COUNT);
  if (rqb_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  // link the list
  rq_p = &rqb_p->list[0];
  for (int i = 1; i < ALLOC_COUNT; ++i) {
    *(uint_ptr*)&rq_p[i - 1] = (uint_ptr)&rq_p[i];
  }

  *(uint_ptr*)&rq_p[ALLOC_COUNT - 1] = (uint_ptr)NULL;

  rqb_p->next_p = rq_list_block_p;
  rq_list_block_p = rqb_p;

  // link to free_rq_list_p
  while (true) {
    free_list_p = free_rq_list_p;
    if (MARKED(free_list_p)) {
      continue;
    }

    if (!compare_and_swap_bool(&free_rq_list_p, free_list_p, MARK(free_list_p))) {
      continue;
    }

    *(uint_ptr*)&rq_p[ALLOC_COUNT - 1] = (uint_ptr)free_list_p;

    if (compare_and_swap_bool((uint_ptr*)&free_rq_list_p, (uint_ptr)MARK(free_list_p), (uint_ptr)&rq_p[0])) {
      break;
    }

    assert(false);
  }

end:

  return error;

#undef ALLOC_COUNT
}

/*
 * static
 */
ResourceQueue *allocResourceQueue()
{
  int error = NO_ERROR;
  ResourceQueue *rq_p = NULL;
  ResourceQueue *rq_next_p = NULL;

  while (true) {
    while (true) {
      rq_p = free_rq_list_p;
      if (rq_p == NULL) {
        break;
      }

      if (MARKED(rq_p) || !compare_and_swap_bool(&free_rq_list_p, rq_p, MARK(rq_p))) {
        continue;
      }

      rq_next_p = (ResourceQueue*)*(uint_ptr*)rq_p;

      if (compare_and_swap_bool((uint_ptr*)&free_rq_list_p, (uint_ptr)MARK(rq_p), (uint_ptr)rq_next_p)) {
        break;
      }

      assert(false);
    }

    if (rq_p == NULL) {
      lock_int(&rq_list_block_lock);

      if (free_rq_list_p == NULL) {
        error = expandRqList();
      }

      unlock_int(&rq_list_block_lock);
    }

    if (error != NO_ERROR || rq_p != NULL) {
      break;
    }
  }

  if (rq_p != NULL) {
    error = initResourceQueue(rq_p);
    if (error != NO_ERROR) {
      freeResourceQueue(rq_p);
      rq_p = NULL;
    }
  }

  return rq_p;
}

/*
 * static
 */
void freeResourceQueue(ResourceQueue *rq_p)
{
  ResourceQueue *rq_next_p = NULL;

  if (rq_p != NULL) {
    while (true) {
      rq_next_p = free_rq_list_p;
      if (MARKED(rq_next_p)) {
        continue;
      }

      *(uint_ptr*)rq_p = (uint_ptr)rq_next_p;

      if (compare_and_swap_bool((uint_ptr*)&free_rq_list_p, (uint_ptr)rq_next_p, (uint_ptr)rq_p)) {
        break;
      }
    }
  }
}

/*
 * static
 */
int wait_func_wrapper(void *arg_p)
{
  int error = NO_ERROR;
  WaitWakeFuncArg *func_arg_p = (WaitWakeFuncArg*)arg_p;

  assert(func_arg_p != NULL
         && func_arg_p->cond_p != NULL && func_arg_p->mutex_p != NULL);

  error = pthread_cond_wait(func_arg_p->cond_p, func_arg_p->mutex_p);
  if (error < 0) {
    error = ER_RQ_LOCK_ERROR;
    SET_ERROR(error);
  }

  return error;
}

/*
 * static
 */
int timed_wait_func_wrapper(void *arg_p)
{
  int error = NO_ERROR;
  WaitWakeFuncArg *func_arg_p = (WaitWakeFuncArg*)arg_p;

  assert(func_arg_p != NULL
         && func_arg_p->cond_p != NULL
         && func_arg_p->mutex_p != NULL && func_arg_p->time_s_p != NULL);

  error = pthread_cond_timedwait(func_arg_p->cond_p, func_arg_p->mutex_p, func_arg_p->time_s_p);
  if (error == ETIMEDOUT) {
    error = ER_RQ_LOCK_TIMEOUT;
    SET_ERROR(error);
  } else if (error < 0) {
    error = ER_RQ_LOCK_ERROR;
    SET_ERROR(error);
  }

  return error;
}

/*
 * static
 */
int wake_func_wrapper(void *arg_p)
{
  int error = NO_ERROR;
  WaitWakeFuncArg *func_arg_p = (WaitWakeFuncArg*)arg_p;

  assert(func_arg_p != NULL && func_arg_p->cond_p != NULL);

  error = pthread_cond_signal(func_arg_p->cond_p);
  if (error < 0) {
    error = ER_RQ_LOCK_ERROR;
    SET_ERROR(error);
  } else if (func_arg_p->error != NO_ERROR) {
    // for waking up the thread for abnormal situations
    error = func_arg_p->error;
  }

  return error;
}

int initResourceQueueEnv()
{
  int error = NO_ERROR;

  rq_list_block_lock = 0;
  error = expandRqList();

  if (error == NO_ERROR) {
    holder_list_block_lock = 0;
    error = expandHolderList();
  }

  return error;
}

void destropyResourceQueueEnv()
{
  void *next_p = NULL;

  while (rq_list_block_p != NULL) {
    next_p = rq_list_block_p->next_p;

    free(rq_list_block_p);

    rq_list_block_p = (ResourceQueueBlock*)next_p;
  }

  while (holder_list_block_p != NULL) {
    next_p = holder_list_block_p->next_p;

    free(holder_list_block_p);

    holder_list_block_p = (RQHolderListBlock*)next_p;
  }
}

/*
 * for test only
 */
int resourceQueueBlockCount()
{
  void *next_p = NULL;
  int i = 0;

  while (rq_list_block_p != NULL) {
    next_p = rq_list_block_p->next_p;

    free(rq_list_block_p);

    rq_list_block_p = (ResourceQueueBlock*)next_p;

    ++i;
  }

  return i;
}

/*
 * for test only
 */
int holderListBlockCount()
{
  void *next_p = NULL;
  int i = 0;

  while (holder_list_block_p != NULL) {
    next_p = holder_list_block_p->next_p;

    free(holder_list_block_p);

    holder_list_block_p = (RQHolderListBlock*)next_p;

    ++i;
  }

  return i;
}

ResourceQueue *createResourceQueue(void *resource_p, const char *name_p)
{
  ResourceQueue *rq_p = NULL;
  int error = NO_ERROR;

  assert(resource_p != NULL);

  rq_p = allocResourceQueue();
  if (rq_p == NULL) {
    goto end;
  }

  if (name_p != NULL) {
    rq_p->name_p = allocString(strlen(name_p), name_p);
    if (rq_p->name_p == NULL) {
      error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
      SET_ERROR(error);

      goto end;
    }
  }

  rq_p->resource.resource_p = resource_p;

end:

  if (error != NO_ERROR && rq_p != NULL) {
    destroyResourceQueueAndSetNull(rq_p);
  }

  return rq_p;
}

void destroyResourceQueue(ResourceQueue *rq_p)
{
  assert(rq_p != NULL);

  if (rq_p->name_p != NULL) {
    freeStringAndSetNull(rq_p->name_p);
  }

  (void)pthread_mutex_destroy(&rq_p->resource.mutex);

  freeResourceQueue(rq_p);
}

int requestResource(ResourceQueue *rq_p, QRRequestType type, void **resource_p)
{
  int error = NO_ERROR;
  RQWaitList *wait_list_p = NULL;
  WaitWakeFuncArg func_arg = { 0 };
  RQThread *thread_p = NULL;

  assert(rq_p != NULL && resource_p != NULL
         && (type == RQ_REQUEST_READ || type == RQ_REQUEST_WRITE));

  wait_list_p = allocHolderList();
  if (wait_list_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  wait_list_p->req_type = type;

  thread_p = getRQThread();
  assert(thread_p != NULL);
  if (thread_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  wait_list_p->subject_p.thread = thread_p;

  error = pthread_mutex_lock(&rq_p->resource.mutex);

  if (error == 0) {
    func_arg.mutex_p = &rq_p->resource.mutex;
    func_arg.cond_p = &thread_p->cond;

    if (!isWaitListEmpty(rq_p)) {
      appendToWaitList(rq_p, wait_list_p);

      error = wait_list_p->subject_p.wait_func_p(&func_arg);

      if (wait_list_p->error != NO_ERROR) {
        error = wait_list_p->error;
        SET_ERROR(error);
      }

      if (error != NO_ERROR) {
        removeFromWaitList(rq_p, wait_list_p);
      }

    } else if (isHolderListEmpty(rq_p)) {
      rq_p->resource.current_type = type;
      appendToHolderList(rq_p, wait_list_p);

    } else if (type == RQ_REQUEST_READ && rq_p->resource.current_type == type) {
      appendToHolderList(rq_p, wait_list_p);

    } else {
      appendToWaitList(rq_p, wait_list_p);

      error = wait_list_p->subject_p.wait_func_p(&func_arg);

      if (wait_list_p->error != NO_ERROR) {
        error = wait_list_p->error;
        SET_ERROR(error);
      }

      if (error != NO_ERROR) {
        removeFromWaitList(rq_p, wait_list_p);
      }

    }
  } else {
    error = ER_RQ_LOCK_ERROR;
    SET_ERROR(error);
  }

  if (error == NO_ERROR) {
    *resource_p = rq_p->resource.resource_p;
  }

end:

  (void)pthread_mutex_unlock(&rq_p->resource.mutex);

  if (error != NO_ERROR && wait_list_p != NULL) {
    freeHolderList(wait_list_p);
  }

  return error;
}

int timedRequestResource(ResourceQueue *rq_p, QRRequestType type, int timeout, void **resource_p)
{
  int error = NO_ERROR;
  RQWaitList *wait_list_p = NULL;
  struct timeval time_v1 = { 0 };
  struct timeval time_v2 = { 0 };
  struct timespec time_s = { 0 };
  int ms = 0;
  WaitWakeFuncArg func_arg = { 0 };
  RQThread *thread_p = NULL;

  assert(rq_p != NULL && resource_p != NULL
         && (type == RQ_REQUEST_READ || type == RQ_REQUEST_WRITE));

  // infinite wait
  if (timeout < 0) {
    return requestResource(rq_p, type, resource_p);
  }

  wait_list_p = allocHolderList();
  if (wait_list_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  wait_list_p->req_type = type;

  (void)gettimeofday(&time_v1, NULL);

  thread_p = getRQThread();
  assert(thread_p != NULL);
  if (thread_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  wait_list_p->subject_p.thread = thread_p;

  while (true) {
    error = pthread_mutex_trylock(&rq_p->resource.mutex);
    if (error == 0) {
      break;
    } else {
      (void)gettimeofday(&time_v2, NULL);

      ms = (time_v2.tv_sec - time_v1.tv_sec) * 1000;
      ms += (time_v2.tv_usec - time_v1.tv_usec)/1000;

      if (ms > timeout) {
        error = ER_RQ_LOCK_TIMEOUT;
        SET_ERROR(error);

        goto end;
      }

      sched_yield();
    }
  }

  if (error == 0) {
    func_arg.mutex_p = &rq_p->resource.mutex;
    func_arg.cond_p = &thread_p->cond;

    (void)gettimeofday(&time_v2, NULL);

    ms = (time_v2.tv_sec - time_v1.tv_sec) * 1000;
    ms += (time_v2.tv_usec - time_v1.tv_usec)/1000;

    if (ms > timeout) {
      error = ER_RQ_LOCK_TIMEOUT;
      SET_ERROR(error);

      goto end;
    }

    time_s.tv_sec = ms / 1000;
    time_s.tv_nsec = (ms % 1000) * 1000000;

    func_arg.time_s_p = &time_s;

    if (!isWaitListEmpty(rq_p)) {
      appendToWaitList(rq_p, wait_list_p);

      error = wait_list_p->subject_p.timed_wait_func_p(&func_arg);

      if (wait_list_p->error != NO_ERROR) {
        error = wait_list_p->error;
        SET_ERROR(error);
      }

      if (error != NO_ERROR) {
        removeFromWaitList(rq_p, wait_list_p);
      }

    } else if (isHolderListEmpty(rq_p)) {
      rq_p->resource.current_type = type;
      appendToHolderList(rq_p, wait_list_p);

    } else if (type == RQ_REQUEST_READ && rq_p->resource.current_type == type) {
      appendToHolderList(rq_p, wait_list_p);

    } else {
      appendToWaitList(rq_p, wait_list_p);

      error = wait_list_p->subject_p.timed_wait_func_p(&func_arg);

      if (wait_list_p->error != NO_ERROR) {
        error = wait_list_p->error;
        SET_ERROR(error);
      }

      if (error != NO_ERROR) {
        removeFromWaitList(rq_p, wait_list_p);
      }
    }
  }

  if (error == NO_ERROR) {
    *resource_p = rq_p->resource.resource_p;
  }

end:

  (void)pthread_mutex_unlock(&rq_p->resource.mutex);

  if (error != NO_ERROR && wait_list_p != NULL) {
    freeHolderList(wait_list_p);
  }

  return error;
}

int releaseResource(ResourceQueue *rq_p)
{
  int error = NO_ERROR;
  bool found = false;
  RQHolderList *holder_list_p = NULL;
  RQWaitList *wait_list_p = NULL;
  WaitWakeFuncArg arg = { 0 };

  assert(rq_p != NULL);

  error = pthread_mutex_lock(&rq_p->resource.mutex);
  if (error < 0) {
    error = ER_RQ_LOCK_ERROR;
    SET_ERROR(error);

    goto end;
  }

  assert(!isHolderListEmpty(rq_p));

  for (holder_list_p = rq_p->holder_list.next_p; holder_list_p != &rq_p->holder_list; holder_list_p = holder_list_p->next_p) {
    assert(holder_list_p->subject_p.thread != NULL);

    if (holder_list_p->subject_p.thread->tid == pthread_self()) {
      found = true;
      break;
    }
  }

  assert(found);

  if (found) {
    removeFromHolderList(rq_p, holder_list_p);
    freeHolderList(holder_list_p);

    arg.error = NO_ERROR;

    while (!isWaitListEmpty(rq_p)) {
      wait_list_p = rq_p->wait_list.next_p;

      assert(wait_list_p->subject_p.thread != NULL);

      arg.cond_p = &wait_list_p->subject_p.thread->cond;

      if (isHolderListEmpty(rq_p)) {
        removeFromWaitList(rq_p, wait_list_p);
        appendToHolderList(rq_p, wait_list_p);

        rq_p->resource.current_type = wait_list_p->req_type;

        wait_list_p->subject_p.wake_func_p(&arg);
      } else if (wait_list_p->req_type == RQ_REQUEST_READ
                 && rq_p->resource.current_type == wait_list_p->req_type) {
        removeFromWaitList(rq_p, wait_list_p);
        appendToHolderList(rq_p, wait_list_p);

        wait_list_p->subject_p.wake_func_p(&arg);
      } else {
        break;
      }
    }
  }

end:

  (void)pthread_mutex_unlock(&rq_p->resource.mutex);

  return error;
}

bool isResourceFree(ResourceQueue *rq_p)
{
  assert(rq_p != NULL);

  if (isHolderListEmpty(rq_p) && isWaitListEmpty(rq_p)) {
    return true;
  }

  return false;
}

/*
 * for abnormal situations
 */
void rqWakeUpAllWaitingThread(ResourceQueue *rq_p)
{
  int error = NO_ERROR;
  RQWaitList *wait_list_p = NULL;
  WaitWakeFuncArg arg = { 0 };

  assert(rq_p != NULL);

  (void)pthread_mutex_lock(&rq_p->resource.mutex);

  arg.error = ER_RQ_ABNORMAL_QUIT;

  while (!isWaitListEmpty(rq_p)) {
    wait_list_p = rq_p->wait_list.next_p;

    assert(wait_list_p->subject_p.thread != NULL);

    arg.cond_p = &wait_list_p->subject_p.thread->cond;

    //removeFromWaitList(rq_p, wait_list_p);

    wait_list_p->error = ER_RQ_ABNORMAL_QUIT;

    wait_list_p->subject_p.wake_func_p(&arg);
  }

  (void)pthread_mutex_unlock(&rq_p->resource.mutex);
}
