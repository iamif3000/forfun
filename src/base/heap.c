/*
 * heap.c
 *
 *  Created on: May 25, 2016
 *      Author: duping
 */

#include <stdlib.h>

#include "../common/common.h"

#include "heap.h"

#define LEAST_HEAP_SIZE 128
#define EXPAND_RATIO 1.1

static void ajustHeap(Heap *heap_p, int64_t length);
static void ajustHeapAppend(Heap *heap_p);
static void initHeap(Heap *heap_p);
static int expandHeap(Heap *heap_p, int64_t size);

/*
 * static
 */
void ajustHeap(Heap *heap_p, int64_t length)
{
  int64_t cur_idx = 0, child_idx = 0;
  int64_t left_child_idx = 0, right_child_idx = 0;
  HeapValue value;

  assert(heap_p != NULL && length >= 0);

  if (length < 2) {
    return;
  }

  cur_idx = 0;
  value = heap_p->base_p[cur_idx];

  while (true) {
    left_child_idx = (cur_idx << 1) + 1;
    right_child_idx = (cur_idx << 1) + 2;

    if (left_child_idx < length) {
      child_idx = left_child_idx;

      if (right_child_idx < length
          && heap_p->cmp_func_p(heap_p->base_p[right_child_idx], heap_p->base_p[left_child_idx]) < 0) {
        child_idx = right_child_idx;
      }
    } else {
      child_idx = length;
    }

    if (child_idx >= length
        || heap_p->cmp_func_p(value, heap_p->base_p[child_idx]) < 0) {
      break;
    }

    heap_p->base_p[cur_idx] = heap_p->base_p[child_idx];

    cur_idx = child_idx;
  }

  heap_p->base_p[cur_idx] = value;
}

/*
 * static
 */
void ajustHeapAppend(Heap *heap_p)
{
  int64_t cur_idx = 0, parent_idx = 0;
  HeapValue value;

  assert(heap_p != NULL);

  if (heap_p->base_used_length < 2) {
    return;
  }

  cur_idx = heap_p->base_used_length - 1;
  value = heap_p->base_p[cur_idx];

  while (true) {
    parent_idx = (cur_idx - 1) >> 1;
    if (parent_idx < 0 || heap_p->cmp_func_p(heap_p->base_p[parent_idx], value) < 0) {
      break;
    }

    heap_p->base_p[cur_idx] = heap_p->base_p[parent_idx];

    cur_idx = parent_idx;
  }

  heap_p->base_p[cur_idx] = value;
}

/*
 * static
 */
void initHeap(Heap *heap_p)
{
  assert(heap_p != NULL);

  heap_p->base_p = NULL;
  heap_p->base_size = 0;
  heap_p->base_used_length = 0;
  heap_p->cmp_func_p = NULL;
}

/*
 * static
 */
int expandHeap(Heap *heap_p, int64_t size)
{
  int error = NO_ERROR;
  HeapValue *new_base_p = NULL;
  HeapValue *old_base_p = NULL;

  assert(heap_p != NULL);

  if (size < heap_p->base_size) {
    goto end;
  }

  new_base_p = (HeapValue*)malloc(sizeof(HeapValue) * size);
  if (new_base_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  old_base_p = heap_p->base_p;
  for (int64_t i = 0; i < heap_p->base_used_length; ++i) {
    new_base_p[i] = old_base_p[i];
  }

  free(old_base_p);

  heap_p->base_p = new_base_p;
  heap_p->base_size = size;

end:

  return error;
}

Heap *createHeap(Heap_cmp_func cmp_func_p, int64_t capacity)
{
  int error = NO_ERROR;
  Heap *heap_p = NULL;

  assert(cmp_func_p != NULL);

  if (capacity < LEAST_HEAP_SIZE) {
    capacity = LEAST_HEAP_SIZE;
  }

  heap_p = (Heap*)malloc(sizeof(Heap));
  if (heap_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  initHeap(heap_p);

  error = expandHeap(heap_p, capacity);
  if (error != NO_ERROR) {
    goto end;
  }

  heap_p->base_size = capacity;
  heap_p->cmp_func_p = cmp_func_p;

end:

  if (error != NO_ERROR && heap_p != NULL) {
    destroyHeapAndSetNull(heap_p);
  }

  return heap_p;
}

void destroyHeap(Heap *heap_p)
{
  if (heap_p != NULL) {
    if (heap_p->base_p != NULL) {
      free(heap_p->base_p);
    }

    free(heap_p);
  }
}

int64_t heapLength(Heap *heap_p)
{
  assert(heap_p != NULL);

  return heap_p->base_used_length;
}

bool isHeapEmpty(Heap *heap_p)
{
  assert(heap_p != NULL);

  return (heap_p->base_used_length < 1);
}

int heapValue(Heap *heap_p, int64_t idx, HeapValue *value_p)
{
  int error = NO_ERROR;

  assert(heap_p != NULL && value_p != NULL);

  if (heap_p->base_used_length <= idx) {
    error = ER_BASE_HEAP_INDEX_OUT_OF_RANGE;
    goto end;
  }

  *value_p = heap_p->base_p[idx];

end:

  return error;
}

int addToHeap(Heap *heap_p, HeapValue value)
{
  int error = NO_ERROR;
  int64_t next_idx = 0;

  assert(heap_p != NULL);

  next_idx = heap_p->base_used_length;
  if (next_idx >= heap_p->base_size) {
    error = expandHeap(heap_p, heap_p->base_size * EXPAND_RATIO);
    if (error != NO_ERROR) {
      goto end;
    }
  }

  heap_p->base_p[next_idx] = value;
  ++heap_p->base_used_length;

  ajustHeapAppend(heap_p);

end:

  return error;
}

int getHeapTop(Heap *heap_p, HeapValue *value_p)
{
  return heapValue(heap_p, 0, value_p);
}

int replaceHeapTop(Heap *heap_p, HeapValue value)
{
  int error = NO_ERROR;

  assert(heap_p != NULL);

  heap_p->base_p[0] = value;

  if (heap_p->base_used_length < 1) {
    heap_p->base_used_length = 1;
  }

  ajustHeap(heap_p, heap_p->base_used_length);

  return error;
}

int deleteHeapTop(Heap *heap_p, HeapValue *value_p)
{
  int error = NO_ERROR;

  assert(heap_p != NULL);

  if (heap_p->base_used_length < 1) {
    error = ER_BASE_HEAP_INDEX_OUT_OF_RANGE;
    goto end;
  }

  if (value_p != NULL) {
    *value_p = heap_p->base_p[0];
  }

  if (heap_p->base_used_length > 1) {
    heap_p->base_p[0] = heap_p->base_p[heap_p->base_used_length - 1];
  }

  if (--heap_p->base_used_length > 1) {
    ajustHeap(heap_p, heap_p->base_used_length);
  }

end:

  return error;
}

int heapSort(Heap *heap_p)
{
  int error = NO_ERROR;
  int64_t i = 0;
  int64_t end_i = 0;
  HeapValue value;

  assert(heap_p != NULL);

  if (heap_p->base_used_length < 2) {
    goto end;
  }

  end_i = heap_p->base_used_length - 1;

  for (i = 0; i < heap_p->base_used_length; ++i) {
    value = heap_p->base_p[0];
    heap_p->base_p[0] = heap_p->base_p[end_i];
    heap_p->base_p[end_i] = value;

    ajustHeap(heap_p, end_i);

    --end_i;
  }

  // make the sort result fits MaxHeap/MinHeap
  i = 0;
  end_i = heap_p->base_used_length - 1;
  while (i < end_i) {
    value = heap_p->base_p[i];
    heap_p->base_p[i] = heap_p->base_p[end_i];
    heap_p->base_p[end_i] = value;

    ++i;
    --end_i;
  }

end:

  return error;
}
