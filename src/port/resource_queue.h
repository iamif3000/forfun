/*
 * resource_queue.h
 *
 *  Created on: Apr 1, 2016
 *      Author: duping
 */

#include <pthread.h>

#include "../base/base_string.h"

#ifndef RESOURCE_QUEUE_H_
#define RESOURCE_QUEUE_H_

// TODO : other system

#define destroyResourceQueueAndSetNull(rq_p) \
  do { \
    destroyResourceQueue(rq_p); \
    rq_p = NULL; \
  } while(0)

typedef struct rq_thread RQThread; // each thread should have it's own RQThread instance
typedef struct rq_subject RQSubject;
typedef struct rq_resource RQResource;
typedef struct rq_holder_list RQHolderList;
typedef struct rq_holder_list RQWaitList;
typedef struct resource_queue ResourceQueue;
typedef enum rq_request_type QRRequestType;

typedef int (*rq_wait_func)(void*);
typedef int (*rq_timed_wait_func)(void*);
typedef int (*rq_wake_func)(void*);

enum rq_request_type {
  RQ_REQUEST_READ,
  RQ_REQUEST_WRITE
};

struct rq_subject {
  RQThread *thread;

  rq_wait_func wait_func_p;
  rq_timed_wait_func timed_wait_func_p;
  rq_wake_func wake_func_p;
};

struct rq_holder_list {
  RQHolderList *next_p;
  RQHolderList *prev_p;

  QRRequestType req_type;

  RQSubject subject_p;
};

struct rq_thread {
  pthread_t tid;
  pthread_cond_t cond;
};

struct rq_resource {
  QRRequestType current_type;
  void *resource_p;
  pthread_mutex_t mutex;
};

struct resource_queue {
  String *name_p;
  RQResource resource;
  RQHolderList holder_list;   // sentinel
  RQWaitList wait_list;       // sentinel
};

int initResourceQueueEnv();
void destropyResourceQueueEnv();

ResourceQueue *createResourceQueue(void *resource_p, const char *name_p);
void destroyResourceQueue(ResourceQueue *rq_p);

int requestResource(ResourceQueue *rq_p, QRRequestType type, void **resource_p);
int timedRequestResource(ResourceQueue *rq_p, QRRequestType type, int timeout, void **resource_p);
int releaseResource(ResourceQueue *rq_p);
bool isResourceFree(ResourceQueue *rq_p);

void rqWakeUpAllWaitingThread(ResourceQueue *rq_p);

/*
 * for test only
 */
int resourceQueueBlockCount();
int holderListBlockCount();

#endif /* RESOURCE_QUEUE_H_ */
