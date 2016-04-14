/*
 * atomic.c
 *
 *  Created on: Mar 11, 2016
 *      Author: duping
 */

#include <assert.h>
#include <stddef.h>
#include <sched.h>

#include "atomic.h"
#include "../common/common.h"

void lock_int(volatile int *i)
{
  assert(i != NULL);

  while (compare_and_swap_value(i, 0, 1) == 1) {
    sched_yield();
  }
}

bool try_lock_int(volatile int *i)
{
  assert(i != NULL);

  return (compare_and_swap_value(i, 0, 1) == 0);
}

void unlock_int(volatile int *i)
{
  assert(i != NULL);

  (void)compare_and_swap_value(i, 1, 0);
}

void lock_rwlock(RWLock *lock, RWLockType type)
{
  bool locked = false;

  assert(lock != NULL && (type == RWLOCK_READ || type == RWLOCK_WRITE));

  while (true) {
    if (type == RWLOCK_READ) {
      if (!lock->is_writing && !lock->about_to_write) {
        lock_int(&lock->lock);

        if (!lock->is_writing  && !lock->about_to_write) {
          ++lock->reader_count;
          locked = true;
        }

        unlock_int(&lock->lock);
      }
    } else {
      if (!lock->is_writing) {
        if (lock->reader_count < 1) {
          lock_int(&lock->lock);

          if (!lock->is_writing) {
            if (lock->reader_count < 1) {
              lock->about_to_write = false;
              lock->is_writing = true;
              locked = true;
            } else {
              lock->about_to_write = true;
            }
          }

          unlock_int(&lock->lock);
        } else {
          lock->about_to_write = true;
        }
      }
    }

    if (locked) {
      break;
    }

    sched_yield();
  }

  //printf("%p lockxx: %d %d %d\n", lock, lock->is_writing, lock->reader_count, type);
}

bool try_lock_rwlock(RWLock *lock, RWLockType type)
{
  bool locked = false;

  assert(lock != NULL && (type == RWLOCK_READ || type == RWLOCK_WRITE));

  if (type == RWLOCK_READ) {
    if (!lock->is_writing && !lock->about_to_write) {
      lock_int(&lock->lock);

      if (!lock->is_writing  && !lock->about_to_write) {
        ++lock->reader_count;
        locked = true;
      }

      unlock_int(&lock->lock);
    }
  } else {
    if (!lock->is_writing && lock->reader_count < 1) {
      lock_int(&lock->lock);

      if (!lock->is_writing && lock->reader_count < 1) {
        lock->about_to_write = false;
        lock->is_writing = true;
        locked = true;
      }

      unlock_int(&lock->lock);
    }
  }

  //printf("%p locktr: %d %d %d %d\n", lock, lock->is_writing, lock->reader_count, type, locked);

  return locked;
}

void unlock_rwlock(RWLock *lock, RWLockType type)
{
  bool break_loop = false;

  assert(lock != NULL && (type == RWLOCK_READ || type == RWLOCK_WRITE));

  lock_int(&lock->lock);

  if (type == RWLOCK_READ) {
    assert(!lock->is_writing && lock->reader_count > 0);

    --lock->reader_count;
  } else {
    assert(lock->is_writing && lock->reader_count < 1);

    lock->is_writing = false;
  }

  unlock_int(&lock->lock);

  //printf("%p unlock: %d %d %d\n", lock, lock->is_writing, lock->reader_count, type);
}

/*
 * NOTE : you must hold the rlock first
 */
void promote_rlock_to_wlock(RWLock *lock)
{
  bool suceed = false;

  assert(lock != NULL);

  while (true) {
    if (!lock->is_writing && lock->reader_count == 1) {
      lock_int(&lock->lock);

      if (!lock->is_writing && lock->reader_count == 1) {
        lock->about_to_write = false;
        lock->is_writing = true;
        --lock->reader_count;

        suceed = true;
      } else if (!lock->about_to_write) {
        lock->about_to_write = true;
      }

      unlock_int(&lock->lock);
    }

    if (suceed) {
      break;
    }

    sched_yield();
  }
}

/*
 * NOTE : you must hold the rlock first
 */
bool try_promote_rlock_to_wlock(RWLock *lock)
{
  bool suceed = false;

  assert(lock != NULL);

  if (!lock->is_writing && lock->reader_count == 1) {
    lock_int(&lock->lock);

    if (!lock->is_writing && lock->reader_count == 1) {
      lock->about_to_write = false;
      lock->is_writing = true;
      --lock->reader_count;

      suceed = true;
    }

    unlock_int(&lock->lock);
  }

  return suceed;
}

/*
 * NOTE : you must hold the wlock first
 */
void demote_wlock_to_rlock(RWLock *lock)
{
  assert(lock != NULL);

  while (!try_demote_wlock_to_rlock(lock)) {
    sched_yield();
  }
}

/*
 * NOTE : you must hold the wlock first
 */
bool try_demote_wlock_to_rlock(RWLock *lock)
{
  bool suceed = false;

  assert(lock != NULL);

  if (lock->is_writing) {
    assert(!lock->about_to_write && lock->reader_count < 1);

    lock_int(&lock->lock);

    if (lock->is_writing) {
      assert(!lock->about_to_write && lock->reader_count < 1);

      lock->is_writing = false;
      ++lock->reader_count;

      suceed = true;
    }

    unlock_int(&lock->lock);
  }

  return suceed;
}
