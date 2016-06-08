#include "../common/id.h"
#include "page.h"

#ifndef SLOT_PAGE_H_
#define SLOT_PAGE_H_

/*
 * NOTE : The first slot (slot 0) is the slot page header
 *        The second slot (slot 1) is the file linker (previous/next page ...)
 *        The third slot (slot 2) is the btree info
 */

#define INIT_SLOT(slot_p) \
  do { \
    slot_p->no = 0; \
    slot_p->type = 0; \
    slot_p->length = 0; \
    slot_p->start = 0; \
    slot_p->end = 0; \
    slot_p->op_id = 0; \
    slot_p->next_pos.page_id.vol_id = NULL_VOLUMEID; \
    slot_p->next_pos.page_id.pg_id = NULL_PAGEIDX; \
    slot_p->next_pos.slot_number = 0; \
  } while(0)


typedef enum slot_type SlotType;
typedef struct slot Slot;
typedef struct slot_page_header SlotPageHeader;
typedef struct slot_page SlotPage;
typedef struct slot_page_record SlotPageRecord;

enum slot_type {
  ALIVE,
  ALIVE_OVERFLOW,
  DEAD,
  DEAD_OVERFLOW,
  VANISHED,
  SLOT_TYPE_COUNT
};

/*
 * May change ...
 * ----------------------------------------------------------------
 * type    | length  | start   | end        | op_id     | next_pos
 * ----------------------------------------------------------------
 * 8 bytes | 8 bytes | 8 bytes | 8 bytes    | 8 bytes   | 24 bytes
 * ----------------------------------------------------------------
 *
 */
struct slot {
  number_t no;
  SlotType type;
  count_t length;     // the real data length
  offset_t start;
  offset_t end;
  number_t op_id;
  SlotID next_pos;
};

struct slot_page_header {
  count_t slot_count;
};

struct slot_page {
  SlotPageHeader header;
  Page *page_p;
};

struct slot_page_record {
  Slot slot;
  count_t length;
  byte *record_p;
};

void formatRawPage(byte *page_p);

int getSlotPageRecord(const Page *page_p, const number_t slot_number, SlotPageRecord *record_p);
int setSlotPageRecord(Page *page_p, const number_t slot_number, const byte *record_p, const count_t record_len);
int updateSlotPageRecord(Page *page_p, const number_t slot_number, const byte *record_p, const count_t record_len);
int appendSlotPageRecord(Page *page_p, const byte *record_p, const count_t record_len);
int allocSlot(Page *page_p, const count_t size, Slot *slot, SlotType type);
int allocOverflowSlot(Page *page_p, Slot *slot);
int setSlotType(Page *page_p, const number_t slot_number, const SlotType type);
int deleteSlot(Page *page_p, const number_t slot_number);

#endif
