/*
 * heap_test.c
 *
 *  Created on: May 27, 2016
 *      Author: duping
 */

#include <stdio.h>
#include <string.h>

#include "../../common/common.h"
#include "../../base/heap.h"

static int compare_int(HeapValue value1, HeapValue value2);

/*
 * static
 */

int compare_int(HeapValue value1, HeapValue value2)
{
  if (value1.i < value2.i) {
    return -1;
  } else if (value1.i > value2.i) {
    return 1;
  }

  return 0;
}

int main(void)
{
#define HEAP_CAP 1024

  int error = NO_ERROR;
  Heap *heap_p = createHeap(compare_int, HEAP_CAP);
  HeapValue value1, value2, value3;

  for (int i = HEAP_CAP - 1; i >= 0; --i) {
    value1.i = i;
    addToHeap(heap_p, value1);
  }

  for (int i = 0; i < HEAP_CAP; ++i) {
    heapValue(heap_p, i, &value1);
    printf("%d ", value1.i);
  }

  printf("\n-------------\n");

  // check parent < two children
  for (int i = 0 ; i <= (HEAP_CAP - 2) >> 1; ++i) {
    heapValue(heap_p, i, &value1);
    heapValue(heap_p, (i << 1) + 1, &value2);
    if ((i << 1) + 2 < heapLength(heap_p)) {
      heapValue(heap_p, (i << 1) + 2, &value3);

      printf ("%d %d %d, ", value1.i, value2.i, value3.i);

      if (value2.i < value1.i || value3.i < value1.i) {
        error = -1;
        goto end;
      }
    } else {
      printf ("%d %d , ", value1.i, value2.i);
      if (value2.i < value1.i) {
        error = -1;
        goto end;
      }
    }
  }

  printf("\n================\n");

  // test sort
  heapSort(heap_p);
  for (int i = 0; i < HEAP_CAP - 1; ++i) {
    heapValue(heap_p, i, &value1);
    heapValue(heap_p, i + 1, &value2);
    if (value2.i < value1.i) {
      error = -2;
      //goto end;
    }
  }

  for (int i = 0; i < HEAP_CAP; ++i) {
    heapValue(heap_p, i, &value1);
    printf("%d ", value1.i);
  }

  printf("\n\n");

end:

  if (error == NO_ERROR) {
    printf("heap test succeed.\n");
  } else {
    printf("heap test failed. %d.\n", error);
  }

  destroyHeap(heap_p);

  return 0;

#undef HEAP_CAP
}
