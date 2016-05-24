/*
 * stack.c
 *
 *  Created on: May 24, 2016
 *      Author: duping
 */

#include "../common/common.h"
#include "stack.h"

#define MIN_STACK_UNIT_CAPACITY 20

Stack *createStack(int unit_capacity)
{
  if (unit_capacity < MIN_STACK_UNIT_CAPACITY) {
    unit_capacity = MIN_STACK_UNIT_CAPACITY;
  }

  return createDeQueue(unit_capacity);
}

void destroyStack(Stack *stack_p)
{
  assert(stack_p != NULL);

  destroyDeQueue(stack_p);
}

int pushIntoStack(Stack *stack_p, StackValue value)
{
  return deQueuePush(stack_p, value);
}

int popOutFromStack(Stack *stack_p, StackValue *value_p)
{
  return deQueuePopFront(stack_p, value_p);
}

bool isStackEmpty(Stack *stack_p)
{
  return isDeQueueEmpty(stack_p);
}
