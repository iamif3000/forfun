/*
 * page_manager.c
 *
 *  Created on: Mar 21, 2016
 *      Author: duping
 */

#include <stdlib.h>
#include <string.h>

#include "../common/common.h"
#include "../base/base_hash.h"

#include "page.h"
#include "volume.h"
#include "page_manager.h"

static PageManager *page_manager_p = NULL;
static volatile int load_page_lock = 0;

static void removeFromLruList(PageCache *pc_p);
static void pushToLruList(PageCache **top_p, PageCache **bottom_p, PageCache *pc_p);
static void pushToLruZoneA(PageCache *pc_p);
static void pushToLruZoneB(PageCache *pc_p);

static PageCache *allocPageCache();
static int initPageCache(PageCache *pc_p);
static void freePageCache(PageCache * pc_p);

/*
 * static
 */
void removeFromLruList(PageCache *pc_p)
{
  PageManager *manager_p = getPageManager();

  assert(pc_p != NULL && manager_p != NULL);

  // zoneA
  if (manager_p->lru_zoneA_top_p == pc_p) {
    manager_p->lru_zoneA_top_p = pc_p->lru_next_p;
  }

  if (manager_p->lru_zoneA_bottom_p == pc_p) {
    manager_p->lru_zoneA_bottom_p = pc_p->lru_prev_p;
  }

  // zoneB
  if (manager_p->lru_zoneB_top_p == pc_p) {
    manager_p->lru_zoneB_top_p = pc_p->lru_next_p;
  }

  if (manager_p->lru_zoneB_bottom_p == pc_p) {
    manager_p->lru_zoneB_bottom_p = pc_p->lru_prev_p;
  }

  // fix the links
  if (pc_p->lru_next_p != NULL) {
    pc_p->lru_next_p->lru_prev_p = pc_p->lru_prev_p;
  }

  if (pc_p->lru_prev_p != NULL) {
    pc_p->lru_prev_p->lru_next_p = pc_p->lru_next_p;
  }

  // cut off
  pc_p->lru_next_p = NULL;
  pc_p->lru_prev_p = NULL;
}

/*
 * static
 */
void pushToLruList(PageCache **top_p, PageCache **bottom_p, PageCache *pc_p)
{
  assert(top_p != NULL && bottom_p != NULL && pc_p != NULL);

  if (*top_p != NULL) {
    assert(*bottom_p != NULL);

    *top_p = pc_p;
    *bottom_p = pc_p;
  } else {
    pc_p->lru_next_p = *top_p;
    (*top_p)->lru_prev_p = pc_p;

    pc_p->lru_prev_p = NULL;
    *top_p = pc_p;
  }
}

/*
 * static
 */
void pushToLruZoneA(PageCache *pc_p)
{
  PageManager *manager_p = getPageManager();

  assert(pc_p != NULL && manager_p != NULL);

  pushToLruList(&manager_p->lru_zoneA_top_p, &manager_p->lru_zoneA_bottom_p, pc_p);
}

/*
 * static
 */
void pushToLruZoneB(PageCache *pc_p)
{
  PageManager *manager_p = getPageManager();

  assert(pc_p != NULL && manager_p != NULL);

  pushToLruList(&manager_p->lru_zoneB_top_p, &manager_p->lru_zoneB_bottom_p, pc_p);
}

/*
 * static
 */
PageCache *allocPageCache()
{
  int error = NO_ERROR;
  PageCache *pc_p = NULL;
  PageManager *manager_p  = getPageManager();

  assert(manager_p != NULL);

  while (true) {
    pc_p = manager_p->page_cache_free_list_p;
    if (pc_p == NULL) {
      break;
    }

    if (MARKED(pc_p) || !compare_and_swap_bool(&manager_p->page_cache_free_list_p, pc_p, MARK(pc_p))) {
      continue;
    }

    if (compare_and_swap_bool(&manager_p->page_cache_free_list_p, MARK(pc_p), pc_p->dirty_next_p)) {
      break;
    }

    assert(false);
  }

  if (pc_p != NULL) {
    error = initPageCache(pc_p);
    if (error != NO_ERROR) {
      freePageCache(pc_p);

      pc_p = NULL;
    }
  }

  return pc_p;
}

/*
 * static
 */
int initPageCache(PageCache *pc_p)
{
  int error = NO_ERROR;

  assert(pc_p != NULL);

  pc_p->dirty_next_p = NULL;
  pc_p->dirty_prev_p = NULL;
  pc_p->lru_next_p = NULL;
  pc_p->lru_prev_p = NULL;

  pc_p->page_res.rq_p = createResourceQueue(&pc_p->page_res.page, NULL);
  if (pc_p->page_res.rq_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);
  }

  return error;
}

/*
 * static
 */
void freePageCache(PageCache * pc_p)
{
  PageManager *manager_p  = getPageManager();
  PageCache *next_p = NULL;

  assert(pc_p != NULL && manager_p != NULL);

  if (pc_p->page_res.rq_p != NULL) {
    destroyResourceQueueAndSetNull(pc_p->page_res.rq_p);
  }

  while (true) {
    next_p = manager_p->page_cache_free_list_p;
    if (MARKED(next_p)) {
      continue;
    }

    pc_p->dirty_next_p = next_p;

    if (compare_and_swap_bool(&manager_p->page_cache_free_list_p, next_p, pc_p)) {
      break;
    }
  }
}

PageManager *getPageManager()
{
  int error = NO_ERROR;
  count_t per_page_cache_size = 0;
  count_t page_cache_count = 0;
  count_t ht_entry_count = 0;
  count_t page_cache_per_ht_entry = 0;

  if (page_manager_p != NULL) {

    goto end;
  } else {
    page_manager_p = (PageManager*)malloc(sizeof(PageManager));
    if (page_manager_p == NULL) {
      error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
      SET_ERROR(error);

      goto end;
    }

    memset(page_manager_p, 0, sizeof(PageManager));

    // calculate the cache size;

    assert((PAGE_SIZE & (PAGE_SIZE - 1)) == 0);

    per_page_cache_size = ALIGN_DEFAULT(sizeof(PageCache) + PAGE_SIZE);
    page_cache_count = ALIGN_BYTES(PAGE_CACHE_SIZE, PAGE_SIZE)/PAGE_SIZE;

    page_manager_p->page_cache_block_p = (PageCache*)malloc(per_page_cache_size * page_cache_count);
    if (page_manager_p->page_cache_block_p == NULL) {
      goto end;
    }

    ht_entry_count = LEAST_HASH_ENTRIES;
    page_cache_per_ht_entry = page_cache_count/ht_entry_count;
    while (page_cache_per_ht_entry > 32) {
      ht_entry_count <<= 1;
      page_cache_per_ht_entry = page_cache_count/ht_entry_count;
    }

    // TODO : hash_func, cmp_func
    page_manager_p->page_cache_ht = createHashTable(ht_entry_count, NULL, NULL, NULL, NULL);
    if (page_manager_p->page_cache_ht == NULL) {
      goto end;
    }

    // init free list
    for (count_t i = 1; i < page_cache_count; ++i) {
      page_manager_p->page_cache_block_p[i - 1].dirty_next_p = &page_manager_p->page_cache_block_p[i];
    }
    page_manager_p->page_cache_block_p[page_cache_count - 1].dirty_next_p = NULL;

    page_manager_p->page_cache_free_list_p = &page_manager_p->page_cache_block_p[0];
  }

end:

  if (error != NO_ERROR) {
    clearPageManager();
    page_manager_p = NULL;
  }

  return page_manager_p;
}

void clearPageManager()
{
  if (page_manager_p != NULL) {
    if (page_manager_p->page_cache_block_p != NULL) {
      free(page_manager_p->page_cache_block_p);
      page_manager_p->page_cache_block_p = NULL;
    }

    if (page_manager_p->dirty_list_p != NULL) {
      // TODO : flush all the dirty pages

      destroyHashTableAndSetNull(page_manager_p->page_cache_ht);
    }
  }
}

Page *fixPage(const PageID page_id, const PageFixType fix_type)
{
  int error = NO_ERROR;
  Page *page_p = NULL;
  HashValue key, value;
  PageManager *manager_p = NULL;
  ResourceQueue *rq_p = NULL;
  QRRequestType lock_type;
  PageCache *page_cache_p = NULL;

  assert(fix_type == FIX_READ || fix_type == FIX_WRITE);

  manager_p = getPageManager();
  if (manager_p == NULL) {
    goto end;
  }

  while (true) {
    key.ptr = (void*)&page_id;

    error = findInHashTable(manager_p->page_cache_ht, key, &value);
    if (error != NO_ERROR) {
      assert(error == ER_HASH_NO_SUCH_KEY);

      lock_int(&load_page_lock);

      // load the page and put it in ht
      error = findInHashTable(manager_p->page_cache_ht, key, &value);
      if (error != NO_ERROR) {
        page_cache_p = allocPageCache();
        if (page_cache_p == NULL) {
          error = ER_PC_NO_MORE_CACHE_ROOM;
          SET_ERROR(error);

          // find victims from lru list
          if (manager_p->lru_zoneB_bottom_p != NULL) {
            lock_int(&manager_p->lru_lock);

            if (manager_p->lru_zoneB_bottom_p != NULL) {
              page_cache_p = manager_p->lru_zoneB_bottom_p;

              key.ptr = &page_cache_p->page_res.page.id;
              error = removeFromHashTable(manager_p->page_cache_ht, key);
              if (error == NO_ERROR) {
                // TODO : consider more about race condition, dirty
                removeFromLruList(page_cache_p);

                page_cache_p->page_res.page.id = page_id;

                error = NO_ERROR;
                removeLastError();
              }
            }

            unlock_int(&manager_p->lru_lock);
          }
        }

        if (error == NO_ERROR) {
          error = loadPage(page_id, &page_cache_p->page_res.page.bytes[0]);
          if (error != NO_ERROR) {
            freePageCache(page_cache_p);
          }
        }

        if (error == NO_ERROR) {
          key.ptr = &page_cache_p->page_res.page.id;
          value.ptr = page_cache_p;

          error = insertToHashTable(manager_p->page_cache_ht, key, value);
          if (error != NO_ERROR) {
            freePageCache(page_cache_p);
          }
        }
      }

      unlock_int(&load_page_lock);

      if (error != NO_ERROR) {
        goto end;
      }
    }

    page_cache_p = (PageCache*)value.ptr;

    // update lru list first
    // not at the top of A list
    if (manager_p->lru_zoneA_top_p != page_cache_p
        && IS_PAGEID_EQUAL(page_id, page_cache_p->page_res.page.id)) {
      lock_int(&manager_p->lru_lock);

      if (manager_p->lru_zoneA_top_p != page_cache_p
          && IS_PAGEID_EQUAL(page_id, page_cache_p->page_res.page.id)) {
        removeFromLruList(page_cache_p);
        pushToLruZoneA(page_cache_p);
      }

      unlock_int(&manager_p->lru_lock);
    }

    // page cache may be removed by lru pick
    if (IS_PAGEID_EQUAL(page_id, page_cache_p->page_res.page.id)) {
      break;
    }
  }

  // latch the page
  rq_p = page_cache_p->page_res.rq_p;
  assert(rq_p != NULL);

  if (fix_type == FIX_READ) {
    lock_type = RQ_REQUEST_READ;
  } else {
    lock_type = RQ_REQUEST_WRITE;
  }

  error = requestResource(rq_p, lock_type, (void**)&page_p);
  if (error != NO_ERROR) {
    page_p = NULL;

    goto end;
  }

end:

  return page_p;
}

void unfixPage(Page *page_p)
{
  PageResource *page_res_p = NULL;
  PageCache *page_cache_p = NULL;
  PageManager *manager_p = getPageManager();

  assert(page_p != NULL && manager_p != NULL);

  page_cache_p = GET_PAGE_CACHE_FROM_PAGE_PTR(page_p);
  page_res_p = &page_cache_p->page_res;

  assert(page_res_p->rq_p != NULL);

  (void)releaseResource(page_res_p->rq_p);

  // update lru list
  tryPushToLruZoneB(page_cache_p);
}

int markPageDirty(Page *page_p)
{
  int error = NO_ERROR;
  PageCache *page_cache_p = NULL, *next_p = NULL;
  PageManager *manager_p = getPageManager();

  assert(page_p != NULL && manager_p != NULL);

  page_cache_p = GET_PAGE_CACHE_FROM_PAGE_PTR(page_p);

  // append to the end
  if (page_cache_p->dirty_next_p == NULL) {
    assert(page_cache_p->dirty_prev_p == NULL);

    lock_int(&manager_p->dirty_lock);

    if (page_cache_p->dirty_next_p == NULL) {
      if (manager_p->dirty_list_p == NULL) {
        manager_p->dirty_list_p = page_cache_p;

        page_cache_p->dirty_next_p = page_cache_p;
        page_cache_p->dirty_prev_p = page_cache_p;
      } else {
        page_cache_p->dirty_next_p = manager_p->dirty_list_p;
        page_cache_p->dirty_prev_p = manager_p->dirty_list_p->dirty_prev_p;
        manager_p->dirty_list_p->dirty_prev_p->dirty_next_p = page_cache_p;
        manager_p->dirty_list_p->dirty_prev_p = page_cache_p;
      }
    }

    unlock_int(&manager_p->dirty_lock);
  }

  return error;
}

PageCache *dequeFromDirtyList()
{
  PageManager *manager_p = getPageManager();
  PageCache *pc_p = NULL;

  assert(pc_p != NULL && manager_p != NULL);

  // remove from the start
  if (manager_p->dirty_list_p != NULL) {
    lock_int(&manager_p->dirty_lock);

    if (manager_p->dirty_list_p != NULL) {
      pc_p = manager_p->dirty_list_p;
      if (pc_p->dirty_next_p == pc_p) {
        manager_p->dirty_list_p = NULL;

        pc_p->dirty_next_p = NULL;
        pc_p->dirty_prev_p = NULL;
      } else {
        manager_p->dirty_list_p = pc_p->dirty_next_p;

        pc_p->dirty_next_p->dirty_prev_p = pc_p->dirty_prev_p;
        pc_p->dirty_prev_p->dirty_next_p = pc_p->dirty_next_p;

        pc_p->dirty_next_p = NULL;
        pc_p->dirty_prev_p = NULL;
      }
    }

    unlock_int(&manager_p->dirty_lock);
  }

  return pc_p;
}

/*
 * NOTE : should be called when holder/waiter change and dirty status change
 */
void tryPushToLruZoneB(PageCache *pc_p)
{
  PageManager *manager_p = getPageManager();
  PageResource *page_res_p;

  assert(pc_p != NULL && manager_p != NULL);

  page_res_p = &pc_p->page_res;

  assert(page_res_p->rq_p != NULL);

  if (isResourceFree(page_res_p->rq_p) && pc_p->dirty_next_p == NULL) {
    lock_int(&manager_p->lru_lock);
    lock_int(&manager_p->dirty_lock);

    if (isResourceFree(page_res_p->rq_p) && pc_p->dirty_next_p == NULL) {
      removeFromLruList(pc_p);
      pushToLruZoneB(pc_p);
    }

    unlock_int(&manager_p->dirty_lock);
    unlock_int(&manager_p->lru_lock);
  }
}
