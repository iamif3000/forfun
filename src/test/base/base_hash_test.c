/*
 * base_hash_test.c
 *
 *  Created on: Mar 22, 2016
 *      Author: duping
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#include "../../common/type.h"
#include "../../common/error.h"
#include "../../port/atomic.h"
#include "../../base/base_hash.h"

#include "../common/test_common.h"

extern int rand_r (unsigned int *__seed);

typedef struct test_arg TestArg;
typedef enum test_action TestAction;
typedef struct test_behavior TestBehavior;
typedef struct test_stat TestStat;

struct test_stat {
  count_t insert_count;
  count_t insert_count_s;
  count_t find_count;
  count_t find_count_s;
  count_t find_l_count;
  count_t find_l_count_s;
  count_t update_count;
  count_t update_count_s;
  count_t remove_count;
  count_t remove_count_s;
  count_t map_count;
  count_t map_count_s;
};

struct test_arg
{
  int thread_no;
  HashTable *ht_p;
  TestStat stat;
};

enum test_action {
  INSERT,
  FIND,
  FIND_WITH_LOCK,
  UPDATE,
  REMOVE,
  MAP,
  LAST_BEHAVIOR
};

struct test_behavior {
  TestAction action;
  int start_key;
  int end_key;
  int start_value;
  int end_value;
};

static TestBehavior behavior_arr[] = {
    { INSERT,           1, 1000, 1, 1000 }
    ,{ FIND,            1, 1000, 1, 2000 }
    ,{ FIND_WITH_LOCK,  1, 1000, 1, 2000 }
    ,{ UPDATE,          1, 1000, 2, 2000 }
    ,{ REMOVE,          1, 1000, 1, 2000 }
    ,{ MAP,             1, 1000, 1, 2000 }
};

static uint64_t hash_int_func(const HashValue key);
static int compare_int_func(const HashValue key1, const HashValue key2);
static void *test_func(void *);
static int map_sum_int_func(void *, HashValue, HashValue);

/*
 * just for test
 * This is not a good hash function
 */
uint64_t hash_int_func(const HashValue key)
{
  uint64_t hash_key = ULONG_MAX + (uint64_t)key.i;
  uint32_t *ui32_p = (uint32_t*)&hash_key;

  hash_key *= ULLONG_MAX;

  ui32_p[0] = (ui32_p[0] >> 15) |  (ui32_p[0] << 17);
  ui32_p[1] = (ui32_p[1] >> 17) |  (ui32_p[0] << 15);

  return hash_key;
}

int compare_int_func(const HashValue key1, const HashValue key2)
{
  int i1, i2;

  i1 = key1.i;
  i2 = key2.i;

  if (i1 > i2) {
    return 1;
  } else if (i1 < i2) {
    return -1;
  }

  return 0;
}

int map_sum_int_func(void *sum_p, HashValue key, HashValue value)
{
  int error = NO_ERROR;
  int *i_p = NULL;

  assert (sum_p != NULL);

  i_p = (int*)sum_p;

  *i_p += value.i;

  return error;
}

void *test_func(void *_arg)
{
  TestArg *arg = (TestArg*)_arg;
  unsigned int seed = 0;
  int r = 0;
  TestBehavior *behavior = NULL;
  int index = 0;
  HashValue key, value;
  int error = NO_ERROR;
  int old_error = NO_ERROR;
  int sum = 0;
  int suceed = 0;
  int fail = 0;
  HashCollisionList *item_p = NULL;

  assert(arg != NULL && arg->ht_p != NULL);

  seed = ULONG_MAX * (unsigned int)arg->thread_no;
  srand(seed);

  for (int i = 0; i < 2000; ++i) {
    r = rand_r(&seed);

    index = r % LAST_BEHAVIOR;
    behavior = &behavior_arr[index];

    r = rand_r(&seed);
    key.i = behavior->start_key + (int)((behavior->end_key - behavior->start_key) * ((double)r/RAND_MAX));

    r = rand_r(&seed);
    value.i = behavior->start_value + (int)((behavior->end_value - behavior->start_value) * ((double)r/RAND_MAX));

    switch (index) {
    case INSERT :
      error = insertToHashTable(arg->ht_p, key, value);

      ++arg->stat.insert_count;
      if (error == NO_ERROR) {
        ++arg->stat.insert_count_s;
      }

      break;
    case FIND :
      error = findInHashTable(arg->ht_p, key, &value);

      ++arg->stat.find_count;
      if (error == NO_ERROR) {
        ++arg->stat.find_count_s;
      }

      break;
    case FIND_WITH_LOCK :
      error = findAndLockInHashTable(arg->ht_p, key, &value, &item_p, HASH_FIND_READ);
      if (error == NO_ERROR) {
        unlockHashTableItem(item_p, HASH_FIND_READ);
      }

      ++arg->stat.find_l_count;
      if (error == NO_ERROR) {
        ++arg->stat.find_l_count_s;
      }

      break;
    case UPDATE :
      error = updateHashTable(arg->ht_p, key, value);

      ++arg->stat.update_count;
      if (error == NO_ERROR) {
        ++arg->stat.update_count_s;
      }

      break;
    case REMOVE :
      error = removeFromHashTable(arg->ht_p, key);

      ++arg->stat.remove_count;
      if (error == NO_ERROR) {
        ++arg->stat.remove_count_s;
      }

      break;
    case MAP :
      sum = 0;
      error = mapHashTable(arg->ht_p, map_sum_int_func, &sum, false, HASH_FIND_READ);

      ++arg->stat.map_count;
      if (error == NO_ERROR) {
        ++arg->stat.map_count_s;
      }

      break;
    default:
      exit(2);
    }

    if (error == NO_ERROR) {
      ++suceed;
    } else {
      ++fail;
    }
  }

  printf("Thread %d: suceed %d, fail %d\n", arg->thread_no, suceed, fail);

  return 0;
}

int main(void)
{
  TestArg *arg_arr[TEST_THREAD_COUNT];
  TestArg test_arg_arr[TEST_THREAD_COUNT];

  const char cstr[] = "Hash table test.";
  HashTable *ht_p = NULL;
  bool failed = false;
  HashBase *base[2];
  int i = 0;
  count_t j = 0;
  HashEntry *entry = NULL;
  HashCollisionList *node = NULL;
  TestStat stat = { 0 };
  int fail_code = 0;


  ht_p = createHashTable(1024, hash_int_func, compare_int_func, NULL, NULL);
  if (ht_p == NULL) {
    exit(1);
  }

  // init
  for (int i = 0; i < TEST_THREAD_COUNT; ++i) {
    test_arg_arr[i].thread_no = i;
    test_arg_arr[i].ht_p = ht_p;
    memset(&test_arg_arr[i].stat, 0, sizeof(test_arg_arr[i].stat));

    arg_arr[i] = &test_arg_arr[i];
  }

  // test
  create_threads_and_run(TEST_THREAD_COUNT, test_func, (void **)arg_arr);

  // check ht status
  if (ht_p->free_list_area_lock != 0
      || ht_p->rehash_lock != 0) {
    failed = true;
    fail_code = 1;
  }

  if (!failed) {
    base[0] = ht_p->current_base_p;
    if (ht_p->is_rehashing) {
      base[1] = ht_p->rehash_insert_base;
    } else {
      base[1] = NULL;
    }

    i = 0;
    while (i < 2 && base[i] != NULL) {
      if (base[i]->update_lock != 0) {
        failed = true;
        fail_code = 2;
        break;
      }

      for (j = 0, entry = base[i]->entries_p; j < base[i]->size; ++j) {
        if (entry->head_lock.lock != 0
            || entry->head_lock.is_writing || entry->head_lock.reader_count > 0) {
          failed = true;
          fail_code = 3;
          break;
        }

        for (node = entry->head_p; node != NULL; node = node->next_p) {
          if (node->rwlock.lock != 0
              || node->rwlock.is_writing || node->rwlock.reader_count > 0) {
            failed = true;
            fail_code = 4;
            break;
          }
        }

        if (failed) {
          break;
        }
      }

      if (failed) {
        break;
      }

      ++i;
    }
  }

  for (i = 0; i < TEST_THREAD_COUNT; ++i) {
    stat.find_count += test_arg_arr[i].stat.find_count;
    stat.find_count_s += test_arg_arr[i].stat.find_count_s;
    stat.find_l_count += test_arg_arr[i].stat.find_l_count;
    stat.find_l_count_s += test_arg_arr[i].stat.find_l_count_s;
    stat.insert_count += test_arg_arr[i].stat.insert_count;
    stat.insert_count_s += test_arg_arr[i].stat.insert_count_s;
    stat.map_count += test_arg_arr[i].stat.map_count;
    stat.map_count_s += test_arg_arr[i].stat.map_count_s;
    stat.remove_count += test_arg_arr[i].stat.remove_count;
    stat.remove_count_s += test_arg_arr[i].stat.remove_count_s;
    stat.update_count += test_arg_arr[i].stat.update_count;
    stat.update_count_s += test_arg_arr[i].stat.update_count_s;
  }

  if (failed) {
    printf("base_hash test FAILS! fail_code : %d\n", fail_code);
  } else {
    printf("base_hash test SUCCESSES!\n"
        "find : %f\nfind_l : %f\ninsert : %f\nmap : %f\nremove : %f\nupdate : %f\n"
        , (float)stat.find_count_s/stat.find_count
        , (float)stat.find_l_count_s/stat.find_l_count
        , (float)stat.insert_count_s/stat.insert_count
        , (float)stat.map_count_s/stat.map_count
        , (float)stat.remove_count_s/stat.remove_count
        , (float)stat.update_count_s/stat.update_count);
  }

  // check ht status

  destroyHashTable(ht_p);

  return 0;
}
