#include "../common/id.h"

#include "../port/resource_queue.h"

#ifndef PAGE_H_
#define PAGE_H_

#define PAGE_SIZE  0x8000 // 16k

#define GET_PAGE_RESOURCE_FROM_PAGE_PTR(page_p) \
  GET_TYPE_PTR_BY_MEMBER_PTR(PageResource, page, page_p)

#define IS_PAGEID_EQUAL(id1, id2) ((id1).vol_id == (id2).vol_id && (id1).pg_id == (id2).pg_id)

typedef struct page Page;
typedef struct page_resource PageResource;

//the size depends on set
struct page {
  PageID id;
  byte bytes[0];
};

struct page_resource {
  ResourceQueue *rq_p;
  Page page;
};

#endif
