/*
 * rbtree_test.c
 *
 *  Created on: May 13, 2016
 *      Author: duping
 */

#include <stdio.h>
#include <string.h>

#include "../../common/common.h"
#include "../../base/rbtree.h"
#include "../../base/base_dequeue.h"

static int compare_int(RBValue v1, RBValue v2);
static void getMaxHeight(RBNode *node_p, int *max_height, int cur_height);

/*
 * static
 */
int compare_int(RBValue v1, RBValue v2)
{
  if (v1.i < v2.i) {
    return -1;
  } else if (v1.i > v2.i) {
    return 1;
  }

  return 0;
}

/*
 * static
 */
void getMaxHeight(RBNode *node_p, int *max_height, int cur_height)
{
  assert(max_height != NULL && cur_height > 0);

  if (node_p == NULL) {
    return;
  }

  if (*max_height < cur_height) {
    *max_height = cur_height;
  }

  getMaxHeight(node_p->left_p, max_height, cur_height + 1);
  getMaxHeight(node_p->right_p, max_height, cur_height + 1);
}

int main(void)
{
  int error = NO_ERROR;
  RBTree *tree_p = NULL;
  DeQueue *dq_p = NULL;
  RBValue key, value;
  int i = 0;

  initRBTreeEnv();

  tree_p = createRBTree(compare_int);
  if (tree_p == NULL) {
    error = -1;
    goto end;
  }

  for (i = 0; i < 1024; ++i) {
    key.i = i;
    value.i = i;
    error = insertIntoRBTree(tree_p, key, value);
    if (error != NO_ERROR) {
      error = -2;
      goto end;
    }
  }

  // check tree height
  int max_height = 0;
  getMaxHeight(tree_p->root_p, &max_height, 1);

  // output the value by mid-order visit
  dq_p = createDeQueue(128);
  DeQueueValue dq_value;
  RBNode *node_p = NULL;

  node_p = tree_p->root_p;
  while (node_p != NULL) {
    dq_value.ptr = node_p;
    deQueuePush(dq_p, dq_value);
    node_p = node_p->left_p;
  }

  while (!isDeQueueEmpty(dq_p)) {
    deQueuePop(dq_p, &dq_value);
    node_p = (RBNode*)dq_value.ptr;
    printf("%d ", node_p->value.i);

    node_p = node_p->right_p;
    if (node_p) {
      dq_value.ptr = node_p;
      deQueuePush(dq_p, dq_value);

      node_p = node_p->left_p;
      while (node_p != NULL) {
        dq_value.ptr = node_p;
        deQueuePush(dq_p, dq_value);
        node_p = node_p->left_p;
      }
    }
  }

  putchar('\n');

  // remove
  for (i = 999; i >= 0; --i) {
    key.i = i;
    (void)removeFromRBTree(tree_p, key);
  }

  // check tree height
  int max_height_2 = 0;
  getMaxHeight(tree_p->root_p, &max_height_2, 1);

  node_p = tree_p->root_p;
  while (node_p != NULL) {
    dq_value.ptr = node_p;
    deQueuePush(dq_p, dq_value);
    node_p = node_p->left_p;
  }

  while (!isDeQueueEmpty(dq_p)) {
    deQueuePop(dq_p, &dq_value);
    node_p = (RBNode*)dq_value.ptr;
    printf("%d ", node_p->value.i);

    node_p = node_p->right_p;
    if (node_p) {
      dq_value.ptr = node_p;
      deQueuePush(dq_p, dq_value);

      node_p = node_p->left_p;
      while (node_p != NULL) {
        dq_value.ptr = node_p;
        deQueuePush(dq_p, dq_value);
        node_p = node_p->left_p;
      }
    }
  }

  putchar('\n');

end:

  destroyDeQueue(dq_p);
  destroyRBTree(tree_p);
  destroyRBTreeEnv();

  if (error == NO_ERROR) {
    printf("rbtree test succeed. %d, %d .\n", max_height, max_height_2);
  } else {
    printf("rbtree test failed.\n");
  }

  return error;
}
