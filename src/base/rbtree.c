/*
 * rbtree.c
 *
 *  Created on: May 10, 2016
 *      Author: duping
 *
 *  Reference http://www.cs.princeton.edu/~rs/talks/LLRB/LLRB.pdf
 *            http://www.teachsolaisgames.com/articles/balanced_left_leaning.html
 */

#include <stdlib.h>
#include <string.h>

#include "../common/common.h"
#include "../common/block_alloc_helper.h"
#include "../port/atomic.h"

#include "queue.h"
#include "rbtree.h"

#define RBNODE_PER_BLOCK 256

#define IS_RED(p) ((p) != NULL && (p)->is_red)

typedef struct rb_node_block RBNodeBlock;

struct rb_node_block {
  RBNodeBlock *next_p;
  RBNode nodes[RBNODE_PER_BLOCK];
};

static volatile int rb_node_block_lock = 0;
static RBNodeBlock *rb_node_block_p = NULL;
static RBNode *free_rb_node_list_p = NULL;

static int allocRBNodeBlock();
static RBNode *allocRBNode();
static void freeRBNode(RBNode *node_p);
static void initRBNode(RBNode *node_p);
static void freeRBTreeNodes(RBNode *root_p);

static RBNode *rotateLeft(RBNode *node_p);
static RBNode *rotateRight(RBNode *node_p);
static void colorFlip(RBNode *node_p);
static RBNode *fixUp(RBNode *node_p);
static RBNode *insertRBNode(RBNode *node_p, RBNode *new_node_p, RB_CMP_FUNC cmp_func_p);
static RBNode *getMin(RBNode *node_p);
static RBNode *deleteMin(RBNode *node_p);
static RBNode *moveRedLeft(RBNode *node_p);
static RBNode *moveRedRight(RBNode *node_p);
static RBNode *deleteRBNode(RBNode *node_p, const RBValue key, RB_CMP_FUNC cmp_func_p);

/*
 * static
 */
int allocRBNodeBlock()
{
  int error = NO_ERROR;
  RBNodeBlock *block_p = NULL;
  RBNode *node_p = NULL, *next_p = NULL;

  block_p = (RBNodeBlock*)malloc(sizeof(RBNodeBlock));
  if (block_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  BAH_LINK_BLOCK(next_p, rb_node_block_p, block_p, nodes, RBNODE_PER_BLOCK, left_p, node_p);
  BAH_LINK_FREE_LIST(left_p, free_rb_node_list_p, &node_p[0], &node_p[RBNODE_PER_BLOCK - 1], next_p);

end:

  return error;
}

/*
 * static
 */
RBNode *allocRBNode()
{
  int error = NO_ERROR;
  RBNode *node_p = NULL;

  while (true) {
    BAH_ALLOC_FROM_FREE_LIST(left_p, free_rb_node_list_p, node_p);

    if (node_p == NULL) {
      BAH_LOCK_AND_ALLOC_BLOCK(rb_node_block_lock, free_rb_node_list_p, allocRBNodeBlock, error);
    } else {
      initRBNode(node_p);
    }

    if (node_p != NULL || error != NO_ERROR) {
      break;
    }
  }

  return node_p;
}

/*
 * static
 */
void freeRBNode(RBNode *node_p)
{
  RBNode *next_p = NULL;

  if (node_p != NULL) {
    BAH_FREE_TO_FREE_LIST(left_p, free_rb_node_list_p, node_p, next_p);
  }
}

/*
 * static
 */
void initRBNode(RBNode *node_p)
{
  assert(node_p != NULL);

  node_p->is_red = true;
  node_p->left_p = NULL;
  node_p->right_p = NULL;

  memset(&node_p->key, 0, sizeof(RBValue));
  memset(&node_p->value, 0, sizeof(RBValue));
}

/*
 * static
 */
void freeRBTreeNodes(RBNode *root_p)
{
  int error = NO_ERROR;
  RBNode *node_p = NULL, *next_p = NULL;
  Queue *q_p = NULL;
  QueueValue value;

  // NOTE : we could free node recursively, here use queue is just for practise.
  if (root_p != NULL) {
    q_p = createQueue(128);
    if (!q_p) {
      // TODO : how to handle the error
      return;
    }

    value.ptr = root_p;
    error = enQueue(q_p, value);
    if (error != NO_ERROR) {
      // TODO : how to handle the error
      return;
    }

    while (!isQueueEmpty(q_p)) {
      error = deQueue(q_p, &value);
      if (error != NO_ERROR) {
        // TODO : how to handle the error
        return;
      }

      node_p = (RBNode*)value.ptr;

      if (node_p->left_p != NULL) {
        value.ptr = node_p->left_p;
        error = enQueue(q_p, value);
        if (error != NO_ERROR) {
          // TODO : how to handle the error
          return;
        }
      }

      if (node_p->right_p != NULL) {
        value.ptr = node_p->right_p;
        error = enQueue(q_p, value);
        if (error != NO_ERROR) {
          // TODO : how to handle the error
          return;
        }
      }

      BAH_FREE_TO_FREE_LIST(left_p, free_rb_node_list_p, node_p, next_p);
    }

    destroyQueue(q_p);
  }
}

/*
 * static
 */
RBNode *rotateLeft(RBNode *node_p)
{
  RBNode *right_p = NULL;

  assert(node_p != NULL && node_p->right_p != NULL);

  right_p = node_p->right_p;
  node_p->right_p = right_p->left_p;
  right_p->left_p = node_p;
  right_p->is_red = node_p->is_red;
  node_p->is_red = true;

  return right_p;
}

/*
 * static
 */
RBNode *rotateRight(RBNode *node_p)
{
  RBNode *left_p = NULL;

  assert(node_p != NULL && node_p->left_p != NULL);

  left_p = node_p->left_p;
  node_p->left_p = left_p->right_p;
  left_p->right_p = node_p;
  left_p->is_red = node_p->is_red;
  node_p->is_red = true;

  return left_p;
}

/*
 * static
 */
void colorFlip(RBNode *node_p)
{
  assert(node_p != NULL);

  node_p->is_red = !node_p->is_red;

  if (node_p->left_p != NULL) {
    node_p->left_p->is_red = !node_p->left_p->is_red;
  }

  if (node_p->right_p != NULL) {
    node_p->right_p->is_red = !node_p->right_p->is_red;
  }
}

/*
 * static
 */
RBNode *fixUp(RBNode *node_p)
{
  assert(node_p != NULL);

  if (IS_RED(node_p->right_p)) {
    node_p = rotateLeft(node_p);
  }

  if (IS_RED(node_p->left_p) && IS_RED(node_p->left_p->left_p)) {
    node_p = rotateRight(node_p);
  }

  if (IS_RED(node_p->left_p) && IS_RED(node_p->right_p)) {
    colorFlip(node_p);
  }

  return node_p;
}

int initRBTreeEnv()
{
  rb_node_block_lock = 0;
  rb_node_block_p = NULL;
  free_rb_node_list_p = NULL;

  return allocRBNodeBlock();
}

void destroyRBTreeEnv()
{
  RBNodeBlock *next_p = NULL;

  while (rb_node_block_p != NULL) {
    next_p = rb_node_block_p->next_p;

    free(rb_node_block_p);

    rb_node_block_p = next_p;
  }

  rb_node_block_p = NULL;
}

RBTree *createRBTree(RB_CMP_FUNC cmp_func)
{
  RBTree *tree_p = NULL;
  RBNode *root_p = NULL;

  assert(cmp_func != NULL);

  tree_p = (RBTree*)malloc(sizeof(RBTree));
  if (tree_p == NULL) {
    goto end;
  }

  // init
  tree_p->root_p = NULL;
  tree_p->cmp_func_p = cmp_func;

end:

  return tree_p;
}

void destroyRBTree(RBTree *tree)
{
  if(tree != NULL) {
    freeRBTreeNodes(tree->root_p);
    free(tree);
  }
}

bool searchRBTree(RBTree *tree_p, const RBValue key, RBValue *value_p)
{
  bool found = false;
  RBNode *node_p = NULL;
  int cmp_result = 0;

  assert(tree_p != NULL && tree_p->cmp_func_p != NULL);

  node_p = tree_p->root_p;

  while (node_p != NULL) {
    cmp_result = tree_p->cmp_func_p(key, node_p->key);
    if (cmp_result < 0) {
      node_p = node_p->left_p;
    } else if (cmp_result > 0) {
      node_p = node_p->right_p;
    } else {
      found = true;
      if (value_p != NULL) {
        *value_p = node_p->value;
      }

      break;
    }
  }

  return found;
}

/*
 * static
 */
RBNode *insertRBNode(RBNode *node_p, RBNode *new_node_p, RB_CMP_FUNC cmp_func_p)
{
  int cmp_result = 0;

  assert(new_node_p != NULL && cmp_func_p != NULL);

  if (node_p == NULL) {
    return new_node_p;
  }

#if defined(USE_234_TREE)
  if (IS_RED(node_p->left_p) && IS_RED(node_p->right_p)) {
    colorFlip(node_p);
  }
#endif

  cmp_result = cmp_func_p(new_node_p->key, node_p->key);
  if (cmp_result == 0) {
    node_p->value = new_node_p->value;
    freeRBNode(new_node_p);
  } else if (cmp_result <= 0) {
    node_p->left_p = insertRBNode(node_p->left_p, new_node_p, cmp_func_p);
  } else {
    node_p->right_p = insertRBNode(node_p->right_p, new_node_p, cmp_func_p);
  }

  if (!IS_RED(node_p->left_p) && IS_RED(node_p->right_p)) {
    node_p = rotateLeft(node_p);
  }

  if (IS_RED(node_p->left_p) && IS_RED(node_p->left_p->left_p)) {
    node_p = rotateRight(node_p);
  }

#if !defined(USE_234_TREE)
  if (IS_RED(node_p->left_p) && IS_RED(node_p->right_p)) {
    colorFlip(node_p);
  }
#endif

  return node_p;
}

int insertIntoRBTree(RBTree *tree_p, const RBValue key, const RBValue value)
{
  int error = NO_ERROR;
  RBNode *new_node_p = NULL;

  assert(tree_p != NULL && tree_p->cmp_func_p != NULL);

  new_node_p = allocRBNode();
  if (new_node_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  new_node_p->key = key;
  new_node_p->value = value;

  tree_p->root_p = insertRBNode(tree_p->root_p, new_node_p, tree_p->cmp_func_p);

end:

  return error;
}

/*
 * static
 */
RBNode *getMin(RBNode *node_p)
{
  assert(node_p != NULL);

  while (node_p->left_p != NULL) {
    node_p = node_p->left_p;
  }

  return node_p;
}

/*
 * static
 */
RBNode *deleteMin(RBNode *node_p)
{
  assert(node_p != NULL);

  if (node_p->left_p == NULL) {
    freeRBNode(node_p);
    return NULL;
  }

  if (!IS_RED(node_p->left_p) && !IS_RED(node_p->left_p->left_p)) {
    node_p = moveRedLeft(node_p);
  }

  node_p->left_p = deleteMin(node_p->left_p);

  return fixUp(node_p);
}

/*
 * static
 */
RBNode *moveRedLeft(RBNode *node_p)
{
  assert(node_p != NULL);

  colorFlip(node_p);

  if (node_p->right_p != NULL && IS_RED(node_p->right_p->left_p)) {
    node_p->right_p = rotateRight(node_p->right_p);
    node_p = rotateLeft(node_p);

    colorFlip(node_p);
  }

  return node_p;
}

/*
 * static
 */
RBNode *moveRedRight(RBNode *node_p)
{
  assert(node_p != NULL);

  if (node_p->left_p != NULL && node_p->right_p != NULL) {
    colorFlip(node_p);

    if (node_p->left_p != NULL && IS_RED(node_p->left_p->left_p)) {
      node_p = rotateRight(node_p);

      colorFlip(node_p);
    }
  }

  return node_p;
}

/*
 * static
 */
RBNode *deleteRBNode(RBNode *node_p, const RBValue key, RB_CMP_FUNC cmp_func_p)
{
  RBNode *min_p = NULL;

  assert(cmp_func_p != NULL);

  if (node_p != NULL) {
    if (cmp_func_p(key, node_p->key) < 0) {
      if (node_p->left_p != NULL && !IS_RED(node_p->left_p) && !IS_RED(node_p->left_p->left_p)) {
        node_p = moveRedLeft(node_p);
      }

      node_p->left_p = deleteRBNode(node_p->left_p, key, cmp_func_p);
    } else {
      if (IS_RED(node_p->left_p)) {
        node_p = rotateRight(node_p);
      }

      if (cmp_func_p(key, node_p->key) == 0 && node_p->right_p == NULL) {
        assert(node_p->left_p == NULL);

        freeRBNode(node_p);
        return NULL;
      }

      if (node_p->right_p != NULL) {
        if (!IS_RED(node_p->right_p) && !IS_RED(node_p->right_p->left_p)) {
          node_p = moveRedRight(node_p);
        }

        if (cmp_func_p(key, node_p->key) == 0) {
          min_p = getMin(node_p->right_p);
          node_p->key = min_p->key;
          node_p->value = min_p->value;

          node_p->right_p = deleteMin(node_p->right_p);
        } else {
          node_p->right_p = deleteRBNode(node_p->right_p, key, cmp_func_p);
        }
      }
    }

    node_p = fixUp(node_p);
  }

  return node_p;
}

int removeFromRBTree(RBTree *tree_p, const RBValue key)
{
  int error = NO_ERROR;
  RBNode *node_p = NULL;
  RBNode *gf_p = NULL, *parent_p = NULL, *uncle_p = NULL;
  int cmp_result = 0;

  assert(tree_p != NULL && tree_p->cmp_func_p != NULL);

  tree_p->root_p = deleteRBNode(tree_p->root_p, key, tree_p->cmp_func_p);
  if (tree_p->root_p != NULL) {
    tree_p->root_p->is_red = false;
  }

  return error;
}
