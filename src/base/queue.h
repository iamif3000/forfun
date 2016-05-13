/*
 * queue.h
 *
 *  Created on: May 11, 2016
 *      Author: duping
 */

#include "../common/type.h"
#include "base_dequeue.h"

#ifndef QUEUE_H_
#define QUEUE_H_

typedef DeQueue Queue;
typedef Value QueueValue;

Queue *createQueue(int unit_capacity);
void destroyQueue(Queue *q_p);

int enQueue(Queue *q_p, QueueValue value);
int deQueue(Queue *q_p, QueueValue *value_p);
bool isQueueEmpty(Queue *q_p);

#endif /* QUEUE_H_ */
