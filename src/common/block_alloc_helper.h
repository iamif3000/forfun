/*
 * block_allocator.h
 *
 *  Created on: May 10, 2016
 *      Author: duping
 */

#ifndef BLOCK_ALLOCATOR_H_
#define BLOCK_ALLOCATOR_H_

/*
 * support block,free_list style allocate and free
 * the params start with capital letter are types or members
 */

#define BAH_LINK_BLOCK(Block_next_p, block_list_p, block_p, Items, items_count, Item_next_p, item_p) \
  do { \
    (block_p)->Block_next_p = (block_list_p); \
    (block_list_p) = (block_p); \
    \
    (item_p) = &block_p->Items[0]; \
    for (count_t i = 0; i < items_count - 1; ++i) { \
      (item_p)[i].Item_next_p = &(item_p)[i + 1]; \
    } \
    \
    (item_p)[items_count - 1].Item_next_p = NULL; \
  } while(0)

#define BAH_LINK_FREE_LIST(Item_next_p, free_item_list_p, first_item_p, last_item_p, next_item_p) \
  while (true) { \
    (next_item_p) = (free_item_list_p); \
    if (MARKED(next_item_p)) { \
      continue; \
    } \
    \
    (last_item_p)->Item_next_p = (next_item_p); \
    \
    if (compare_and_swap_bool(&(free_item_list_p), (next_item_p), (first_item_p))) { \
      break; \
    } \
  }

#define BAH_FREE_TO_FREE_LIST(Item_next_p, free_item_list_p, item_p, next_item_p) \
  while (true) { \
    (next_item_p) = (free_item_list_p); \
    if (MARKED(next_item_p)) { \
      continue; \
    } \
    \
    (item_p)->Item_next_p = (next_item_p); \
    \
    if (compare_and_swap_bool(&(free_item_list_p), (next_item_p), (item_p))) { \
      break; \
    } \
  }

#define BAH_ALLOC_FROM_FREE_LIST(Item_next_p, free_item_list_p, item_p) \
  while (true) { \
    (item_p) = (free_item_list_p); \
    if ((item_p) == NULL) { \
      break; \
    } \
    \
    if (MARKED((item_p)) || !compare_and_swap_bool(&(free_item_list_p), (item_p), MARK(item_p))) { \
      continue; \
    } \
    \
    if (compare_and_swap_bool(&(free_item_list_p), MARK(item_p), (item_p)->Item_next_p)) { \
      break; \
    } \
    \
    assert(false); \
  }

#define BAH_LOCK_AND_ALLOC_BLOCK(free_item_list_lock, free_item_list_p, alloc_func_p, error) \
  do { \
    lock_int(&(free_item_list_lock)); \
    \
    if ((free_item_list_p) == NULL) { \
      (error) = (alloc_func_p)(); \
    } \
    \
    unlock_int(&(free_item_list_lock)); \
  } while(0)

#endif /* BLOCK_ALLOCATOR_H_ */
