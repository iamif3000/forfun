/*
 * rbtree.h
 *
 *  Created on: May 10, 2016
 *      Author: duping
 *
 *  Reference http://www.cs.princeton.edu/~rs/talks/LLRB/LLRB.pdf
 *            http://www.teachsolaisgames.com/articles/balanced_left_leaning.html
 *
 *  TODO : make test. I'm not sure this is correct.
 */

#include "../common/type.h"

#ifndef RBTREE_H_
#define RBTREE_H_

#define destroyRBTreeAndSetNULL(p) \
  do { \
    destroyRBTree(p); \
    (p) = NULL; \
  } while(0)

typedef rb_color RBColor;
typedef Value RBValue;
typedef struct rb_node RBNode;
typedef struct rb_tree RBTree;
typedef int (*RB_CMP_FUNC)(RBValue, RBValue);

struct rb_node {
  bool is_red;
  RBNode *left_p;
  RBNode *right_p;
  RBValue key;
  RBValue value;
};

struct rb_tree {
  RBNode *root_p;
  RB_CMP_FUNC cmp_func_p;
};

// call init before any create
int initRBTreeEnv();
void destroyRBTreeEnv();

RBTree *createRBTree(RB_CMP_FUNC *cmp_func);
void destroyRBTree(RBTree *tree_p);

bool searchRBTree(RBTree *tree_p, const RBValue key, RBValue *value_p);
int insertIntoRBTree(RBTree *tree_p, const RBValue key, const RBValue value);
int removeFromRBTree(RBTree *tree_p, const RBValue key);

#endif /* RBTREE_H_ */
