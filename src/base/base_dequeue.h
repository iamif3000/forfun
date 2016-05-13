/*
 * base_queue.h
 *
 *  Created on: May 10, 2016
 *      Author: duping
 */

#include "../common/type.h"

#ifndef BASE_DEQUEUE_H_
#define BASE_DEQUEUE_H_

#define destroyDeQueueAndSetNULL(p) \
  do { \
    destroyDeQueue(p); \
    (p) = NULL; \
  } while(0)

typedef Value DeQueueValue;
typedef struct dequeu_unit DeQueueUnit;
typedef struct dequeue DeQueue;

struct dequeu_unit {
  DeQueueUnit *next_p;
  DeQueueUnit *prev_p;
  int front_index;
  int end_index;
  DeQueueValue values[0];
};

struct dequeue {
  volatile int lock;
  int unit_values_count;
  DeQueueUnit *front_p;
  DeQueueUnit *end_p;
  DeQueueUnit *free_list_p;
};

DeQueue *createDeQueue(int unit_capacity);
void destroyDeQueue(DeQueue *dq_p);

int deQueuePush(DeQueue *dq_p, DeQueueValue value);
int deQueuePop(DeQueue *dq_p, DeQueueValue *value_p);
int deQueuePushFront(DeQueue *dq_p, DeQueueValue value);
int deQueuePopFront(DeQueue *dq_p, DeQueueValue *value_p);
bool isDeQueueEmpty(DeQueue *dq_p);

#endif /* BASE_QUEUE_H_ */
