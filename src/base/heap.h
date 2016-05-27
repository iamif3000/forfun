/*
 * heap.h
 *
 *  Created on: May 25, 2016
 *      Author: duping
 */

#include "../common/type.h"

#ifndef HEAP_H_
#define HEAP_H_

/*
 * NOTE : Heap_cmp_func return -1 for (1 < 2) creates MinHeap
 *        Heap_cmp_func return -1 for (2 > 1) creates MaxHeap
 */

#define destroyHeapAndSetNull(p) \
  do { \
    destroyHeap(p); \
    (p) = NULL; \
  } while (0)

typedef Value HeapValue;
typedef struct heap Heap;
typedef int (*Heap_cmp_func)(HeapValue value1, HeapValue value2);

struct heap {
  HeapValue *base_p;
  Heap_cmp_func cmp_func_p;
  int64_t base_size;
  int64_t base_used_length;
};

Heap *createHeap(Heap_cmp_func cmp_func_p, int64_t capacity);
void destroyHeap(Heap *heap_p);

int64_t heapLength(Heap *heap_p);
bool isHeapEmpty(Heap *heap_p);
int heapValue(Heap *heap_p, int64_t idx, HeapValue *value_p);

int addToHeap(Heap *heap_p, HeapValue value);
int getHeapTop(Heap *heap_p, HeapValue *value_p);
int replaceHeapTop(Heap *heap_p, HeapValue value);
int deleteHeapTop(Heap *heap_p, HeapValue *value_p);

int heapSort(Heap *heap_p);

#endif /* HEAP_H_ */
