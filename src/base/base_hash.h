/*
 * base_hash.h
 *
 *  Created on: Mar 21, 2016
 *      Author: duping
 */

#include "../common/type.h"
#include "../port/atomic.h"

#ifndef BASE_HASH_H_
#define BASE_HASH_H_

// TODO : make these parameters configurable
#define LEAST_HASH_ENTRIES 1024
#define ENTRIES_INCREASE_RATIO 1.0
#define FREE_COLLISION_LIST_ALLOC 1024
#define COLLISION_COUNT_THRESHOLD 100
#define REHASH_RATIO 0.2
#define REHASH_RETRY 10
#define REHASH_STEPS 100

#define destroyHashTableAndSetNull(hash_tab_p) \
  do { \
    destroyHashTable(hash_tab_p); \
    (hash_tab_p) = NULL; \
  } while(0)

typedef enum hash_find_lock_type HashFindLockType;
typedef Value HashValue;
typedef struct hash_value_pair HashValuePair;
typedef struct hash_collision_list HashCollisionList;
typedef struct hash_entry HashEntry;
typedef struct hash_base HashBase;
typedef struct hash_collision_list_area HashCollisionListArea;
typedef struct hash_table HashTable;

typedef uint64_t (*HashFunc)(const HashValue);
typedef int (*MapFunc)(void *, HashValue, HashValue);
typedef MapFunc FreeFunc;
typedef int (*CompareFunc)(const HashValue, const HashValue);

enum hash_find_lock_type {
  HASH_FIND_READ,
  HASH_FIND_WRITE
};

struct hash_value_pair {
  HashValue key;
  HashValue value;
};

struct hash_collision_list {
  HashCollisionList *next_p;
  RWLock rwlock;
  HashValuePair pair;
};

struct hash_entry {
  HashCollisionList *head_p;
  RWLock head_lock;
  count_t list_length;
};

struct hash_base {
  HashEntry *entries_p;
  count_t size;
  count_t items_count;
  count_t collision_count;
  volatile int update_lock;
};

struct hash_collision_list_area {
  HashCollisionListArea *next_p;
  HashCollisionList list_items[0];
};

struct hash_table {
  HashBase *current_base_p;
  HashBase base1;
  HashBase base2;

  HashCollisionListArea *free_list_area;
  HashCollisionList *free_list;
  volatile int free_list_area_lock;

  HashFunc hash_func_p;
  CompareFunc cmp_func_p;
  FreeFunc free_func_p;
  void *free_func_arg;

  uint64_t rehash_pos;
  HashBase *rehash_insert_base;
  bool is_rehashing;
  count_t pause_rehash_count;
  volatile int rehash_lock;
};

HashTable *createHashTable(count_t reserve_entries, HashFunc hash_func_p, CompareFunc cmp_func_p, FreeFunc free_func_p, void *free_func_arg);
void destroyHashTable(HashTable *hash_tab_p);

int insertToHashTable(HashTable *hash_tab_p, const HashValue key, const HashValue value);
int findInHashTable(HashTable *hash_tab_p, const HashValue key, HashValue *value);

int findAndLockInHashTable(HashTable *hash_tab_p, const HashValue key, HashValue *value, HashCollisionList **item_p, const HashFindLockType type);
void unlockHashTableItem(HashCollisionList *item_p, const HashFindLockType type);

int updateHashTable(HashTable *hash_tab_p, const HashValue key, const HashValue new_value);
int removeFromHashTable(HashTable *hash_tab_p, const HashValue key);
int mapHashTable(HashTable *hash_tab_p, MapFunc map_func_p, void *arg, const bool ignore_error, const HashFindLockType type);

#endif /* BASE_HASH_H_ */
