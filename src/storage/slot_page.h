#include "../common/id.h"
#include "page.h"

#ifndef SLOT_PAGE_H_
#define SLOT_PAGE_H_


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

int getSlotPageRecord(const SlotPage *slot_page_p, const number_t slot_number, SlotPageRecord *record_p);
int setSlotPageRecord(SlotPage *slot_page_p, const number_t slot_number, const byte *record_p, const count_t record_len);
int updateSlotPageRecord(SlotPage *slot_page_p, const number_t slot_number, const byte *record_p, const count_t record_len);
int appendSlotPageRecord(SlotPage *slot_page_p, const byte *record_p, const count_t record_len);
int allocSlot(SlotPage *slot_page_p, const count_t size, Slot *slot, SlotType type);
int allocOverflowSlot(SlotPage *slot_page_p, Slot *slot);
int setSlotType(SlotPage *slot_page_p, const number_t slot_number, const SlotType type);
int deleteSlot(SlotPage *slot_page_p, const number_t slot_number);

#endif
