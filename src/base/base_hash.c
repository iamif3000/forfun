/*
 * base_hash.c
 *
 *  Created on: Mar 21, 2016
 *      Author: duping
 */

#include <stdlib.h>
#include <string.h>

#include "../common/common.h"
#include "../common/block_alloc_helper.h"
#include "../port/atomic.h"
#include "base_hash.h"

typedef int (*FIND_DO_FUNC)(HashCollisionList *node_p, void *arg);
typedef struct find_lock_arg FindLockArg;
typedef struct update_arg UpdateArg;

struct find_lock_arg {
  HashValue *value_p;
  HashCollisionList **item_p;
  RWLockType lock_type;
};

struct update_arg {
  HashTable *hash_tab_p;
  HashValue new_value;
};

static int insertToHashTableImp(HashTable *hash_tab_p, HashBase *insert_base, const HashValue key, const HashValue value);
static HashCollisionList *allocCollisionListNode(HashTable *hash_tab_p);
static void freeCollisionListNode(HashTable *hash_tab_p, HashCollisionList *node_p);
static void rehashNSteps(HashTable *hash_tab_p, int n);
static void doRehash(HashTable *hash_tab_p);
static void pauseRehash(HashTable *hash_tab_p);
static void resumeRehash(HashTable *hash_tab_p);
static bool canRehash(HashTable *hash_tab_p);
static int findAndDo(HashTable *hash_tab_p, const HashValue key, FIND_DO_FUNC do_func, void *arg, bool lock_head);
static void initHashEntry(HashEntry *entry);

// FIND_DO_FUNC functions
static int findInHashTableDoFunc(HashCollisionList *node_p, void *arg);
static int findAndLockInHashTableDoFunc(HashCollisionList *node_p, void *arg);
static int updateHashTableDoFunc(HashCollisionList *node_p, void *arg);

/*
 * static
 */
void initHashEntry(HashEntry *entry)
{
  assert(entry != NULL);

  entry->list_length = 0;
  entry->head_p = NULL;

  INIT_RWLOCK(&entry->head_lock);
}

/*
 * static
 */
int insertToHashTableImp(HashTable *hash_tab_p, HashBase *insert_base, const HashValue key, const HashValue value)
{
  int error = NO_ERROR;
  uint64_t hash_key = 0;
  uint64_t entry_idx = 0;
  HashEntry *entry_p = NULL;
  HashCollisionList *node_p = NULL, *new_node_p = NULL;
  bool change_collision = false;
  bool rehash = false;

  assert(hash_tab_p != NULL && hash_tab_p->hash_func_p != NULL
      && hash_tab_p->cmp_func_p != NULL && insert_base != NULL);

  hash_key = hash_tab_p->hash_func_p(key);

  entry_idx = hash_key % insert_base->size;
  entry_p = &insert_base->entries_p[entry_idx];

  // check for duplicate
  error = findInHashTable(hash_tab_p, key, NULL);
  if (error == NO_ERROR) {
    error = ER_HASH_DUPLICATE_KEY;
    SET_ERROR(error);
  }

  if (error != ER_HASH_NO_SUCH_KEY) {
    goto end;
  }

  new_node_p = allocCollisionListNode(hash_tab_p);
  if (new_node_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  new_node_p->pair.key = key;
  new_node_p->pair.value = value;

  // update collision list
  lock_rwlock(&entry_p->head_lock, RWLOCK_WRITE);

  // search the list again for duplicate
  error = findAndDo(hash_tab_p, key, findInHashTableDoFunc, NULL, false);
  if (error == NO_ERROR) {
    error = ER_HASH_DUPLICATE_KEY;
    SET_ERROR(error);
  }

  if (error == ER_HASH_NO_SUCH_KEY) {
    error = NO_ERROR;
    removeLastError();

    new_node_p->next_p = entry_p->head_p;
    entry_p->head_p = new_node_p;

    ++entry_p->list_length;
  }

  unlock_rwlock(&entry_p->head_lock, RWLOCK_WRITE);

  if (error != NO_ERROR) {
    goto end;
  }

  // do not free the node when error, from now on
  new_node_p = NULL;

  // update base stat info
  lock_int(&insert_base->update_lock);

  ++insert_base->items_count;

  if (change_collision) {
    ++insert_base->collision_count;
  }

  if ((double)insert_base->collision_count / insert_base->items_count > REHASH_RATIO) {
    rehash = true;
  }

  unlock_int(&insert_base->update_lock);

  // set rehash
  if (rehash && !hash_tab_p->is_rehashing && try_lock_int(&hash_tab_p->rehash_lock)) {
    if (!hash_tab_p->is_rehashing) {
      if (hash_tab_p->current_base_p == &hash_tab_p->base1) {
        hash_tab_p->rehash_insert_base = &hash_tab_p->base2;
      } else {
        hash_tab_p->rehash_insert_base = &hash_tab_p->base1;
      }

      // realloc the entries for base
      HashEntry *new_entries_p = NULL;
      count_t size = hash_tab_p->rehash_insert_base->size;
      if (size < LEAST_HASH_ENTRIES) {
        size = LEAST_HASH_ENTRIES;
      }

      count_t new_size = (count_t)(size * (1.0 + ENTRIES_INCREASE_RATIO));

      new_entries_p = (HashEntry*)malloc(sizeof(HashEntry) * new_size);
      if (new_entries_p != NULL) {
        count_t i = 0;
        HashEntry *old_entries_p = hash_tab_p->rehash_insert_base->entries_p;

        if (old_entries_p != NULL) {
          for (; i < size; ++i) {
            assert(old_entries_p[i].list_length < 1
                && !old_entries_p[i].head_lock.is_writing
                && !old_entries_p[i].head_lock.about_to_write
                && old_entries_p[i].head_lock.lock < 1
                && old_entries_p[i].head_lock.reader_count < 1);

            initHashEntry(&new_entries_p[i]);
          }
        }

        for (; i < new_size; ++i) {
          initHashEntry(&new_entries_p[i]);
        }

        // free old entries and setup new entries
        free(old_entries_p);

        hash_tab_p->rehash_insert_base->entries_p = new_entries_p;
        hash_tab_p->rehash_insert_base->size = new_size;

        hash_tab_p->rehash_pos = 0;
        hash_tab_p->is_rehashing = true;
      } else {
        // let error pass through
        error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
        SET_ERROR(error);
      }
    }

    unlock_int(&hash_tab_p->rehash_lock);
  }

end:

  if (error != NO_ERROR && new_node_p != NULL) {
    freeCollisionListNode(hash_tab_p, new_node_p);
  }

  return error;
}

/*
 * static
 */
HashCollisionList *allocCollisionListNode(HashTable *hash_tab_p)
{
  HashCollisionList *node_p = NULL;
  HashCollisionListArea *area_p = NULL;
  HashCollisionList *free_list_p = NULL;

  assert(hash_tab_p != NULL);

  while (true) {
    BAH_ALLOC_FROM_FREE_LIST(next_p, hash_tab_p->free_list, node_p)

    if (node_p == NULL) {
      area_p = NULL;

      lock_int(&hash_tab_p->free_list_area_lock);

      if (hash_tab_p->free_list == NULL) {
        area_p = (HashCollisionListArea*)malloc(sizeof(HashCollisionListArea) + sizeof(HashCollisionList) * FREE_COLLISION_LIST_ALLOC);
        if (area_p != NULL) {
          BAH_LINK_BLOCK(next_p, hash_tab_p->free_list_area, area_p, list_items, FREE_COLLISION_LIST_ALLOC, next_p, free_list_p);
          BAH_LINK_FREE_LIST(next_p, hash_tab_p->free_list, &area_p->list_items[0], &area_p->list_items[FREE_COLLISION_LIST_ALLOC - 1], free_list_p);
        }
      } else {
        // somebody else allocated the area
        area_p = hash_tab_p->free_list_area;
      }

      unlock_int(&hash_tab_p->free_list_area_lock);

      if (area_p == NULL) {
        break;
      }
    } else {
      node_p->next_p = NULL;
      INIT_RWLOCK(&node_p->rwlock);

      break;
    }
  }

  return node_p;
}

/*
 * static
 */
void freeCollisionListNode(HashTable *hash_tab_p, HashCollisionList *node_p)
{
  HashCollisionList *free_list_p = NULL;

  assert(hash_tab_p != NULL && node_p != NULL);

  BAH_FREE_TO_FREE_LIST(next_p, hash_tab_p->free_list, node_p, free_list_p);
}

/*
 * static
 * NOTE : rehash_lock must be hold
 */
void rehashNSteps(HashTable *hash_tab_p, int n)
{
  uint64_t entry_idx = 0;
  HashBase *current_base_p = NULL, *insert_base_p = NULL;
  HashEntry *entries_p = NULL, *insert_entry_p = NULL, *remove_entry_p = NULL;
  int i = 0;
  int retry = 0;
  HashCollisionList *node_p = NULL, *next_p = NULL;
  bool head_locked = false;
  bool base_locked = false;
  uint64_t hash_key = 0;

  assert(hash_tab_p != NULL);

  entry_idx = hash_tab_p->rehash_pos;
  current_base_p = hash_tab_p->current_base_p;
  insert_base_p = hash_tab_p->rehash_insert_base;

  assert(entry_idx < current_base_p->size);

  entries_p = current_base_p->entries_p;

  while (entry_idx < current_base_p->size && i < n && retry < REHASH_RETRY) {
    remove_entry_p = &entries_p[entry_idx];
    if (try_lock_rwlock(&remove_entry_p->head_lock, RWLOCK_WRITE)) {
      head_locked = true;

      if (try_lock_int(&current_base_p->update_lock)) {
        node_p = remove_entry_p->head_p;
        if (node_p != NULL && try_lock_rwlock(&node_p->rwlock, RWLOCK_WRITE)) {
          // insert node to insert_base
          hash_key = hash_tab_p->hash_func_p(node_p->pair.key);
          insert_entry_p = &insert_base_p->entries_p[hash_key % insert_base_p->size];

          if (try_lock_rwlock(&insert_entry_p->head_lock, RWLOCK_WRITE)) {
            // reset retry
            retry = 0;

            // remove from old list
            remove_entry_p->head_p = node_p->next_p;

            --remove_entry_p->list_length;

            // release remove entry lock
            unlock_rwlock(&remove_entry_p->head_lock, RWLOCK_WRITE);
            head_locked = false;

            // insert to new list
            node_p->next_p = insert_entry_p->head_p;
            insert_entry_p->head_p = node_p;

            ++insert_entry_p->list_length;

            unlock_rwlock(&insert_entry_p->head_lock, RWLOCK_WRITE);

            --current_base_p->items_count;
          }

          unlock_rwlock(&node_p->rwlock, RWLOCK_WRITE);
        } else if (node_p == NULL) {
          // no item in this entry
          if (++hash_tab_p->rehash_pos >= current_base_p->size) {
            hash_tab_p->rehash_pos = 0;
          }

          entry_idx = hash_tab_p->rehash_pos;
        } else {
          ++retry;
        }

        // break the loop
        if (current_base_p->items_count < 1) {
          i = n;
          hash_tab_p->current_base_p = hash_tab_p->rehash_insert_base;
        }

        unlock_int(&current_base_p->update_lock);

        ++i;
      } else {
        ++retry;
      }

      if (head_locked) {
        unlock_rwlock(&remove_entry_p->head_lock, RWLOCK_WRITE);
        head_locked = false;
      }
    } else {
      ++retry;
    }
  }
}

/*
 * static
 */
void doRehash(HashTable *hash_tab_p)
{
  assert(hash_tab_p != NULL);

  if (hash_tab_p->is_rehashing && canRehash(hash_tab_p) && try_lock_int(&hash_tab_p->rehash_lock)) {
    if (hash_tab_p->is_rehashing && canRehash(hash_tab_p)) {
      // do rehash n steps, until no data in current_base_p
      rehashNSteps(hash_tab_p, REHASH_STEPS);
    }

    unlock_int(&hash_tab_p->rehash_lock);
  }
}

/*
 * static
 */
void pauseRehash(HashTable *hash_tab_p)
{
  assert(hash_tab_p != NULL);

  // TODO : consider more
//  lock_int(&hash_tab_p->rehash_lock);
//  ++hash_tab_p->pause_rehash_count;
//  unlock_int(&hash_tab_p->rehash_lock);
//  atomic_add(&hash_tab_p->pause_rehash_count, 1);
}

/*
 * static
 */
void resumeRehash(HashTable *hash_tab_p)
{
  assert(hash_tab_p != NULL);

//  lock_int(&hash_tab_p->rehash_lock);
//  --hash_tab_p->pause_rehash_count;
//  unlock_int(&hash_tab_p->rehash_lock);
//  atomic_sub(&hash_tab_p->pause_rehash_count, 1);
}

/*
 * static
 */
bool canRehash(HashTable *hash_tab_p)
{
  assert(hash_tab_p != NULL);

  return (hash_tab_p->pause_rehash_count < 1);
}

/*
 * static
 */
int findAndDo(HashTable *hash_tab_p, const HashValue key, FIND_DO_FUNC do_func, void *arg, bool lock_head)
{
  int error = NO_ERROR;
  uint64_t hash_key;
  int base_idx = 0;
  HashBase *base[2] = { 0 };
  HashEntry *entry = NULL;
  bool found = false;

  assert(hash_tab_p != NULL && hash_tab_p->hash_func_p != NULL && hash_tab_p->cmp_func_p != NULL && do_func != NULL);

  doRehash(hash_tab_p);

  pauseRehash(hash_tab_p);

  base[0] = hash_tab_p->current_base_p;
  if (hash_tab_p->is_rehashing) {
    base[1] = hash_tab_p->rehash_insert_base;
  } else {
    base[1] = NULL;
  }

  hash_key = hash_tab_p->hash_func_p(key);

  while (base_idx < 2 && base[base_idx] != NULL && !found) {
    entry = &base[base_idx]->entries_p[hash_key % base[base_idx]->size];

    if (lock_head) {
      lock_rwlock(&entry->head_lock, RWLOCK_READ);
    }

    for (HashCollisionList *p = entry->head_p; p != NULL; p = p->next_p) {
      if (hash_tab_p->cmp_func_p(p->pair.key, key) == 0) {
        error = do_func(p, arg);
        found = true;

        break;
      }
    }

    if (lock_head) {
      unlock_rwlock(&entry->head_lock, RWLOCK_READ);
    }

    ++base_idx;
  }

  resumeRehash(hash_tab_p);

  if (error == NO_ERROR && !found) {
    error = ER_HASH_NO_SUCH_KEY;
    SET_ERROR(error);
  }

  return error;
}

HashTable *createHashTable(count_t reserve_entries, HashFunc hash_func_p, CompareFunc cmp_func_p, FreeFunc free_func_p, void *free_func_arg)
{
  int error = NO_ERROR;
  HashTable *htable_p = NULL;
  HashBase *hbase_p = NULL;

  assert(hash_func_p != NULL && cmp_func_p != NULL);

  if (reserve_entries < LEAST_HASH_ENTRIES) {
    reserve_entries = LEAST_HASH_ENTRIES;
  }

  htable_p = (HashTable*)malloc(sizeof(HashTable));
  if (htable_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  // set all fields to 0
  memset(htable_p, 0, sizeof(HashTable));

  htable_p->free_list_area = (HashCollisionListArea*)malloc(sizeof(HashCollisionListArea) + sizeof(HashCollisionList) * FREE_COLLISION_LIST_ALLOC);
  if (htable_p->free_list_area == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  htable_p->free_list_area->next_p = NULL;

  // init free list
  HashCollisionList *tmp_list = htable_p->free_list_area->list_items;
  for (int i = 0; i < FREE_COLLISION_LIST_ALLOC - 1; ++i) {
    tmp_list[i].next_p = &tmp_list[i+1];
  }

  tmp_list[FREE_COLLISION_LIST_ALLOC - 1].next_p = htable_p->free_list;
  htable_p->free_list = &tmp_list[0];

  // init base1
  hbase_p = &htable_p->base1;
  hbase_p->entries_p = (HashEntry*)malloc(sizeof(HashEntry) * reserve_entries);
  if (hbase_p->entries_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  for (int i = 0; i < reserve_entries; ++i) {
    initHashEntry(&hbase_p->entries_p[i]);
  }

  hbase_p->collision_count = 0;
  hbase_p->items_count = 0;
  hbase_p->size = reserve_entries;
  hbase_p->update_lock = 0;

  // init base2
  hbase_p = &htable_p->base2;
  hbase_p->entries_p = NULL;
  hbase_p->collision_count = 0;
  hbase_p->items_count = 0;
  hbase_p->size = 0;
  hbase_p->update_lock = 0;

  // set base
  htable_p->current_base_p = &htable_p->base1;
  htable_p->rehash_insert_base = NULL;

  // init locks
  htable_p->free_list_area_lock = 0;
  htable_p->rehash_lock = 0;

  // setup funcs
  htable_p->hash_func_p = hash_func_p;
  htable_p->cmp_func_p = cmp_func_p;
  htable_p->free_func_p = free_func_p;
  htable_p->free_func_arg = free_func_arg;

end:

  if (error != NO_ERROR && htable_p != NULL) {
    destroyHashTable(htable_p);
    htable_p = NULL;
  }

  return htable_p;
}

void destroyHashTable(HashTable *hash_tab_p)
{
  void *to_free = NULL;
  void *next = NULL;

  assert(hash_tab_p != NULL);

  if (hash_tab_p->free_func_p != NULL) {
    (void)mapHashTable(hash_tab_p, hash_tab_p->free_func_p, hash_tab_p->free_func_arg, true, HASH_FIND_WRITE);
  }

  to_free = hash_tab_p->base1.entries_p;
  if (to_free != NULL) {
    free(to_free);
  }

  to_free = hash_tab_p->base2.entries_p;
  if (to_free != NULL) {
    free(to_free);
  }

  to_free = hash_tab_p->free_list_area;
  while (to_free != NULL) {
    next = ((HashCollisionListArea*)to_free)->next_p;
    free(to_free);
    to_free = next;
  }

  free(hash_tab_p);
}

int insertToHashTable(HashTable *hash_tab_p, const HashValue key, const HashValue value)
{
  int error = NO_ERROR;
  HashBase *insert_base = NULL;
  count_t rehash_entries = 0;

  assert(hash_tab_p != NULL);

  doRehash(hash_tab_p);

  pauseRehash(hash_tab_p);

  if (hash_tab_p->is_rehashing) {
    insert_base = hash_tab_p->rehash_insert_base;
  } else {
    insert_base = hash_tab_p->current_base_p;
  }

  error = insertToHashTableImp(hash_tab_p, insert_base, key, value);

  resumeRehash(hash_tab_p);

  return error;
}

/*
 * static
 */
int findInHashTableDoFunc(HashCollisionList *node_p, void *arg)
{
  HashValue *value = NULL;

  assert(node_p != NULL);

  if (arg != NULL) {
    value = (HashValue*)arg;
    *value = node_p->pair.value;
  }

  return NO_ERROR;
}

/*
 * NOTE : won't check the lock of items(HashCollisionList)
 * if you have senario that need lock,
 * see findInHashTableLock, findInHashTableTryLock and findInHashTableUnlock
 */
int findInHashTable(HashTable *hash_tab_p, const HashValue key, HashValue *value)
{
  int error = NO_ERROR;

  assert(hash_tab_p != NULL && hash_tab_p->hash_func_p != NULL && hash_tab_p->cmp_func_p != NULL);

  error = findAndDo(hash_tab_p, key, findInHashTableDoFunc, value, true);

  return error;
}

/*
 * static
 */
int findAndLockInHashTableDoFunc(HashCollisionList *node_p, void *arg)
{
  FindLockArg *find_lock_arg = NULL;

  assert(node_p != NULL && arg != NULL);

  find_lock_arg = (FindLockArg*)arg;

  lock_rwlock(&node_p->rwlock, find_lock_arg->lock_type);

  *find_lock_arg->item_p = node_p;
  if (find_lock_arg->value_p != NULL) {
    *find_lock_arg->value_p = node_p->pair.value;
  }

  return NO_ERROR;
}

/*
 * NOTE : do not modify item_p. It's for release the lock
 */
int findAndLockInHashTable(HashTable *hash_tab_p, const HashValue key, HashValue *value, HashCollisionList **item_p, const HashFindLockType type)
{
  int error = NO_ERROR;
  RWLockType lock_type;
  FindLockArg find_lock_arg;

  assert(hash_tab_p != NULL && hash_tab_p->hash_func_p != NULL
      && hash_tab_p->cmp_func_p != NULL && item_p != NULL
      && (type == HASH_FIND_READ || type == HASH_FIND_WRITE));

  if (type == HASH_FIND_READ) {
    lock_type = RWLOCK_READ;
  } else {
    lock_type = RWLOCK_WRITE;
  }

  find_lock_arg.lock_type = lock_type;
  find_lock_arg.value_p = value;
  find_lock_arg.item_p = item_p;

  error = findAndDo(hash_tab_p, key, findAndLockInHashTableDoFunc, &find_lock_arg, true);

  return error;
}

void unlockHashTableItem(HashCollisionList *item_p, const HashFindLockType type)
{
  assert(item_p != NULL && (type == HASH_FIND_READ || type == HASH_FIND_WRITE));

  unlock_rwlock(&item_p->rwlock, type);
}

/*
 * static
 */
int updateHashTableDoFunc(HashCollisionList *node_p, void *arg)
{
  UpdateArg *update_arg_p = NULL;
  HashTable *hash_tab_p = NULL;

  assert(node_p != NULL && arg != NULL);

  update_arg_p = (UpdateArg*)arg;
  hash_tab_p = update_arg_p->hash_tab_p;

  lock_rwlock(&node_p->rwlock, RWLOCK_WRITE);

  if (hash_tab_p->free_func_p != NULL) {
    hash_tab_p->free_func_p(hash_tab_p->free_func_arg, (HashValue)0, node_p->pair.value);
  }

  node_p->pair.value = update_arg_p->new_value;

  unlock_rwlock(&node_p->rwlock, RWLOCK_WRITE);

  return NO_ERROR;
}

int updateHashTable(HashTable *hash_tab_p, const HashValue key, const HashValue new_value)
{
  int error = NO_ERROR;
  UpdateArg update_arg;

  assert(hash_tab_p != NULL && hash_tab_p->hash_func_p != NULL && hash_tab_p->cmp_func_p != NULL);

  update_arg.hash_tab_p = hash_tab_p;
  update_arg.new_value = new_value;

  error = findAndDo(hash_tab_p, key, updateHashTableDoFunc, &update_arg, true);

  return error;
}

int removeFromHashTable(HashTable *hash_tab_p, const HashValue key)
{
  int error = NO_ERROR;
  uint64_t hash_key;
  int base_idx = 0;
  HashBase *base[2] = { 0 };
  HashEntry *entry = NULL;
  bool found = false;
  bool locked = false;
  bool change_collision = false;
  HashCollisionList *free_list_p = NULL;

  assert(hash_tab_p != NULL && hash_tab_p->hash_func_p != NULL && hash_tab_p->cmp_func_p != NULL);

  // pause rehash
  doRehash(hash_tab_p);

  pauseRehash(hash_tab_p);

  base[0] = hash_tab_p->current_base_p;

  if (hash_tab_p->is_rehashing) {
    base[1] = hash_tab_p->rehash_insert_base;
  } else {
    base[1] = NULL;
  }

  hash_key = hash_tab_p->hash_func_p(key);

  // first check current_base
  while (base_idx < 2 && base[base_idx] != NULL && !found) {
    entry = &base[base_idx]->entries_p[hash_key % base[base_idx]->size];

    lock_rwlock(&entry->head_lock, RWLOCK_WRITE);
    locked = true;

    for (HashCollisionList *p = entry->head_p, *prev = NULL; p != NULL; prev = p, p = p->next_p) {
      if (hash_tab_p->cmp_func_p(p->pair.key, key) == 0) {
        lock_rwlock(&p->rwlock, RWLOCK_WRITE);

        if (hash_tab_p->free_func_p != NULL) {
          hash_tab_p->free_func_p(hash_tab_p->free_func_arg, p->pair.key, p->pair.value);
        }

        // remove the node from list
        if (prev == NULL) {
          entry->head_p = p->next_p;
        } else {
          prev->next_p = p->next_p;
        }

        unlock_rwlock(&p->rwlock, RWLOCK_WRITE);

        if (entry->list_length > COLLISION_COUNT_THRESHOLD && !change_collision) {
          change_collision = true;
        }

        --entry->list_length;

        unlock_rwlock(&entry->head_lock, RWLOCK_WRITE);
        locked = false;

        // update base stat
        lock_int(&base[base_idx]->update_lock);

        if (change_collision) {
          --base[base_idx]->collision_count;
        }
        --base[base_idx]->items_count;

        unlock_int(&base[base_idx]->update_lock);

        BAH_FREE_TO_FREE_LIST(next_p, hash_tab_p->free_list, p, free_list_p);

        found = true;
      }

      if (found) {
        break;
      }
    }

    if (locked) {
      unlock_rwlock(&entry->head_lock, RWLOCK_WRITE);
      locked = false;
    }

    ++base_idx;
  }

  if (!found) {
    error = ER_HASH_NO_SUCH_KEY;
    SET_ERROR(error);
  }

  resumeRehash(hash_tab_p);

  return error;
}

int mapHashTable(HashTable *hash_tab_p, MapFunc map_func_p, void *arg, const bool ignore_error, const HashFindLockType type)
{
  int error = NO_ERROR;
  uint64_t hash_key;
  int base_idx = 0;
  HashBase *base[2] = { 0 };
  HashEntry *entry = NULL;
  bool found = false;
  RWLockType lock_type;

  assert(hash_tab_p != NULL && map_func_p != NULL
      && (type == HASH_FIND_READ || type == HASH_FIND_WRITE));

  if (type == HASH_FIND_READ) {
    lock_type = RWLOCK_READ;
  } else {
    lock_type = RWLOCK_WRITE;
  }

  doRehash(hash_tab_p);

  pauseRehash(hash_tab_p);

  base[0] = hash_tab_p->current_base_p;
  if (hash_tab_p->is_rehashing) {
    base[1] = hash_tab_p->rehash_insert_base;
  } else {
    base[1] = NULL;
  }

  // first check current_base
  while (base_idx < 2 && base[base_idx] != NULL) {
    if (base[base_idx]->items_count > 0) {
      for (count_t i = 0; i < base[base_idx]->size; ++i) {
        entry = &base[base_idx]->entries_p[i];

        lock_rwlock(&entry->head_lock, RWLOCK_READ);

        for (HashCollisionList *p = entry->head_p; p != NULL; p = p->next_p) {
          lock_rwlock(&p->rwlock, lock_type);

          error = map_func_p(arg, p->pair.key, p->pair.value);

          unlock_rwlock(&p->rwlock, lock_type);

          if (error != NO_ERROR) {
            if (ignore_error) {
              error = NO_ERROR;
              removeLastError();
            } else {
              break;
            }
          }
        }

        unlock_rwlock(&entry->head_lock, RWLOCK_READ);
      }

      if (error != NO_ERROR) {
        break;
      }
    }

    ++base_idx;
  }

  resumeRehash(hash_tab_p);

  return error;
}
