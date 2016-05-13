/*
 * base_queue.c
 *
 *  Created on: May 10, 2016
 *      Author: duping
 */

#include <stdlib.h>

#include "../common/common.h"
#include "../port/atomic.h"

#include "base_dequeue.h"

#define MIN_UNIT_CAPACITY 20

static void initDeQueue(DeQueue *dq_p, int unit_capacity);
static void initDeQueueUnit(DeQueueUnit *dqu_p);
static int expandDeQueue(DeQueue *dq_p, bool expand_end);

/*
 * static
 */
void initDeQueue(DeQueue *dq_p, int unit_capacity)
{
  assert(dq_p != NULL);

  if (unit_capacity < MIN_UNIT_CAPACITY) {
    unit_capacity = MIN_UNIT_CAPACITY;
  }

  dq_p->lock = 0;
  dq_p->front_p = NULL;
  dq_p->end_p = NULL;
  dq_p->free_list_p = NULL;
  dq_p->unit_values_count = unit_capacity;
}

/*
 * static
 */
void initDeQueueUnit(DeQueueUnit *dqu_p)
{
  assert(dqu_p != NULL);

  dqu_p->front_index = 0;
  dqu_p->end_index = 0;
  dqu_p->next_p = NULL;
  dqu_p->prev_p = NULL;
}

/*
 * static
 * NOTE : lock must be hold to make the expand safe
 */
int expandDeQueue(DeQueue *dq_p, bool expand_end)
{
  int error = NO_ERROR;
  DeQueueUnit *unit_p = NULL;
  count_t unit_size = 0;

  assert(dq_p != NULL);

  if (dq_p->free_list_p == NULL) {
    unit_size = sizeof(DeQueueUnit) + sizeof(DeQueueValue) * dq_p->unit_values_count;
    unit_size = ALIGN_DEFAULT(unit_size);

    unit_p = (DeQueueUnit*)malloc(unit_size);
    if (unit_p == NULL) {
      error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
      goto end;
    }
  } else {
    unit_p = dq_p->free_list_p;
    dq_p->free_list_p = unit_p->next_p;
  }

  initDeQueueUnit(unit_p);

  if (dq_p->front_p == NULL) {
    assert(dq_p->end_p == NULL);

    dq_p->front_p = unit_p;
    dq_p->end_p = unit_p;

    // first alloc
    unit_p->front_index = dq_p->unit_values_count >> 1;
    unit_p->end_index = dq_p->unit_values_count >> 1;
  } else {
    assert(dq_p->end_p != NULL && dq_p->lock == 1);

    if (expand_end) {
      dq_p->end_p->next_p = unit_p;
      unit_p->prev_p = dq_p->end_p;

      dq_p->end_p = unit_p;
    } else {
      dq_p->front_p->prev_p = unit_p;
      unit_p->next_p = dq_p->front_p;

      dq_p->front_p = unit_p;

      // expand front
      unit_p->front_index = dq_p->unit_values_count;
      unit_p->end_index = dq_p->unit_values_count;
    }
  }

end:

  return error;
}

DeQueue *createDeQueue(int unit_capacity)
{
  int error = NO_ERROR;
  DeQueue *dq_p = NULL;

  dq_p = (DeQueue*)malloc(sizeof(DeQueue));
  if (dq_p == NULL) {
    goto end;
  }

  initDeQueue(dq_p, unit_capacity);
  error = expandDeQueue(dq_p, true);
  if (error != NO_ERROR) {
    goto end;
  }

end:

  if (error != NO_ERROR && dq_p != NULL) {
    destroyDeQueueAndSetNULL(dq_p);
  }

  return dq_p;
}
void destroyDeQueue(DeQueue *dq_p)
{
  DeQueueUnit *next_p = NULL;

  if (dq_p != NULL) {
    while (dq_p->front_p != NULL) {
      next_p = dq_p->front_p->next_p;

      free(dq_p->front_p);

      dq_p->front_p = next_p;
    }

    while (dq_p->free_list_p != NULL) {
      next_p = dq_p->free_list_p->next_p;

      free(dq_p->free_list_p);

      dq_p->free_list_p = next_p;
    }

    free(dq_p);
  }
}

/*
 * push to current end_index and end_index++
 */
int deQueuePush(DeQueue *dq_p, DeQueueValue value)
{
  int error = NO_ERROR;
  DeQueueUnit *unit_p = NULL;

  assert(dq_p != NULL && dq_p->front_p != NULL && dq_p->end_p != NULL);

  lock_int(&dq_p->lock);

  unit_p = dq_p->end_p;
  if (unit_p->end_index >= dq_p->unit_values_count) {
    error = expandDeQueue(dq_p, true);
    if (error != NO_ERROR) {
      goto end;
    }

    unit_p = dq_p->end_p;
  }

  unit_p->values[unit_p->end_index++] = value;

end:

  unlock_int(&dq_p->lock);

  return error;
}

int deQueuePop(DeQueue *dq_p, DeQueueValue *value_p)
{
  int error = NO_ERROR;
  DeQueueUnit *unit_p = NULL;

  assert(dq_p != NULL && dq_p->front_p != NULL && dq_p->end_p != NULL && value_p != NULL);

  lock_int(&dq_p->lock);

  unit_p = dq_p->end_p;
  if (unit_p->front_index >= unit_p->end_index) {
    error = ER_BASE_DEQUEUE_NO_MORE_ITEM;
    goto end;
  }

  *value_p = unit_p->values[--unit_p->end_index];
  if (dq_p->front_p != dq_p->end_p && unit_p->end_index < 1) {
    // move to the previous unit
    unit_p = dq_p->end_p;
    unit_p->prev_p->next_p = NULL;
    dq_p->end_p = unit_p->prev_p;

    // release unit_p to free list
    unit_p->next_p = dq_p->free_list_p;
    dq_p->free_list_p = unit_p;
  }

end:

  unlock_int(&dq_p->lock);

  return error;
}

/*
 * current front_index-- and push to front_index
 */
int deQueuePushFront(DeQueue *dq_p, DeQueueValue value)
{
  int error = NO_ERROR;
  DeQueueUnit *unit_p = NULL;

  assert(dq_p != NULL && dq_p->front_p != NULL && dq_p->end_p != NULL);

  lock_int(&dq_p->lock);

  unit_p = dq_p->front_p;
  if (unit_p->front_index < 1) {
    error = expandDeQueue(dq_p, false);
    if (error != NO_ERROR) {
      goto end;
    }

    unit_p = dq_p->front_p;
  }

  unit_p->values[--unit_p->front_index] = value;

end:

  unlock_int(&dq_p->lock);

  return error;
}

int deQueuePopFront(DeQueue *dq_p, DeQueueValue *value_p)
{
  int error = NO_ERROR;
  DeQueueUnit *unit_p = NULL;

  assert(dq_p != NULL && dq_p->front_p != NULL && dq_p->end_p != NULL && value_p != NULL);

  lock_int(&dq_p->lock);

  unit_p = dq_p->front_p;
  if (unit_p->front_index >= unit_p->end_index) {
    error = ER_BASE_DEQUEUE_NO_MORE_ITEM;
    goto end;
  }

  *value_p = unit_p->values[unit_p->front_index++];
  if (dq_p->front_p != dq_p->end_p && unit_p->front_index >= dq_p->unit_values_count) {
    // move to the next unit
    unit_p = dq_p->front_p;
    unit_p->next_p->prev_p = NULL;
    dq_p->front_p = unit_p->next_p;

    // put in free list
    unit_p->next_p = dq_p->free_list_p;
    dq_p->free_list_p = unit_p;
  }

end:

  unlock_int(&dq_p->lock);

  return error;
}

bool isDeQueueEmpty(DeQueue *dq_p)
{
  bool is_empty = false;

  assert(dq_p != NULL && dq_p->front_p != NULL && dq_p->end_p != NULL);

  lock_int(&dq_p->lock);

  if (dq_p->front_p == dq_p->end_p && dq_p->front_p->front_index == dq_p->front_p->end_index) {
    is_empty = true;
  }

  unlock_int(&dq_p->lock);

  return is_empty;
}

