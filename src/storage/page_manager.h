/*
 * page_manager.c
 *
 *  Created on: Mar 21, 2016
 *      Author: duping
 */

#include "../port/resource_queue.h"

#ifndef PAGE_MANAGER_C_
#define PAGE_MANAGER_C_

// TODO : make PAGE_CACHE_SIZE configurable

#define PAGE_CACHE_SIZE 0x20000000 // 512M

#define GET_PAGE_CACHE_FROM_PAGE_PTR(page_p) \
  GET_TYPE_PTR_BY_MEMBER_PTR(PageCache, page_res, GET_PAGE_RESOURCE_FROM_PAGE_PTR(page_p))

typedef struct page_cache PageCache;
typedef struct page_manager PageManager;
typedef enum page_fix_type PageFixType;

struct page_cache {
  PageCache *lru_next_p;
  PageCache *lru_prev_p;
  PageCache *dirty_next_p;  // also free list
  PageCache *dirty_prev_p;
  PageResource page_res;
};

struct page_manager {
  volatile int lru_lock;
  volatile int dirty_lock;

  HashTable *page_cache_ht;         // PageID --> PageCache

  PageCache *page_cache_block_p;    // fix size
  PageCache *page_cache_free_list_p;

  PageCache *lru_zoneA_top_p;       // holder + waiter count >= 1 or dirty
  PageCache *lru_zoneA_bottom_p;
  PageCache *lru_zoneB_top_p;       // holder + waiter count < 1 and clean
  PageCache *lru_zoneB_bottom_p;

  PageCache *dirty_list_p;          // there should be a thread to flush pages
};

enum page_fix_type {
  FIX_READ,
  FIX_WRITE
};

PageManager *getPageManager();
void clearPageManager();

Page *fixPage(const PageID page_id, const PageFixType fix_type);
void unfixPage(Page *page_p);

int markPageDirty(Page *page_p);

PageCache *dequeFromDirtyList();
void tryPushToLruZoneB(PageCache *pc_p);

#endif /* PAGE_MANAGER_C_ */
