/*
 * queue.c
 *
 *  Created on: May 11, 2016
 *      Author: duping
 */

#include "../common/common.h"
#include "queue.h"

#define MIN_QUEUE_UNIT_CAPACITY 20

Queue *createQueue(int unit_capacity)
{
  if (unit_capacity < MIN_QUEUE_UNIT_CAPACITY) {
    unit_capacity = MIN_QUEUE_UNIT_CAPACITY;
  }

  return createDeQueue(MIN_QUEUE_UNIT_CAPACITY);
}

void destroyQueue(Queue *q_p)
{
  assert(q_p != NULL);

  destroyDeQueue(q_p);
}

int enQueue(Queue *q_p, QueueValue value)
{
  return deQueuePush(q_p, value);
}

int deQueue(Queue *q_p, QueueValue *value_p)
{
  return deQueuePopFront(q_p, value_p);
}

bool isQueueEmpty(Queue *q_p)
{
  return isDeQueueEmpty(q_p);
}
