/*
 * stack.h
 *
 *  Created on: May 24, 2016
 *      Author: duping
 */

#include "../common/type.h"
#include "base_dequeue.h"

#ifndef STACK_H_
#define STACK_H_

typedef DeQueue Stack;
typedef Value StackValue;

Stack *createStack(int unit_capacity);
void destroyStack(Stack *stack_p);

int pushIntoStack(Stack *stack_p, StackValue value);
int popOutFromStack(Stack *stack_p, StackValue *value_p);
bool isStackEmpty(Stack *stack_p);

#endif /* STACK_H_ */
