/*
 * atomic.h
 *
 *  Created on: Mar 11, 2016
 *      Author: duping
 */

#include "../common/type.h"

#ifndef ATOMIC_H_
#define ATOMIC_H_

#define MARK(p) ((uint_ptr)(p) | 0x01)
#define UNMARK(p) ((uint_ptr)(p) & ~0x01)
#define MARKED(p) ((uint_ptr)(p) & 0x01)

// TODO: other platform/compiler
#define compare_and_swap_value(p, old_val, new_val) __sync_val_compare_and_swap(p, old_val, new_val)
#define compare_and_swap_bool(p, old_val, new_val) __sync_bool_compare_and_swap(p, old_val, new_val)
#define atomic_add(p, value) __sync_add_and_fetch (p, value)
#define atomic_sub(p, value) __sync_sub_and_fetch (p, value)

#define memory_barrier __sync_synchronize
#define INIT_RWLOCK(rwlock_p) \
  do { \
    (rwlock_p)->lock = 0; \
    (rwlock_p)->about_to_write = false; \
    (rwlock_p)->is_writing = false; \
    (rwlock_p)->reader_count = 0; \
  } while(0)

typedef enum read_write_lock_type RWLockType;
typedef struct read_write_lock RWLock;

enum read_write_lock_type {
  RWLOCK_READ,
  RWLOCK_WRITE
};

struct read_write_lock {
  volatile int lock;
  bool about_to_write;
  bool is_writing;
  count_t reader_count;
};

void lock_int(volatile int *i);
bool try_lock_int(volatile int *i);
void unlock_int(volatile int *i);

void lock_rwlock(RWLock *lock, RWLockType type);
bool try_lock_rwlock(RWLock *lock, RWLockType type);
void unlock_rwlock(RWLock *lock, RWLockType type);

void promote_rlock_to_wlock(RWLock *lock);
bool try_promote_rlock_to_wlock(RWLock *lock);
void demote_wlock_to_rlock(RWLock *lock);
bool try_demote_wlock_to_rlock(RWLock *lock);

#endif /* ATOMIC_H_ */
