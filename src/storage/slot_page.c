#include <string.h>
#include "../common/common.h"
#include "slot_page.h"

// TODO: wal

#define HEADER_SLOT 0
#define FILE_SLOT 1
#define SLOT_PAGE_HEADER_SIZE 8
#define SLOT_SIZE 64 //8 + 8 + 8 + 8

static int getSlot(const Page *page_p, const number_t slot_number, Slot *slot);
static int setSlot(Page *page_p, const number_t slot_number, const Slot *slot);
static void getSlotPageHeader(Page *page_p, SlotPageHeader *header);
static void setSlotPageHeader(Page *page_p, const SlotPageHeader *header);
static count_t getSlotPageLeftSize(const Page *page_p);

//static
int getSlot(const Page *page_p, const number_t slot_number, Slot *slot)
{
#define READ_64BITS_FROM_STREAM(stream, type, ref) \
  do { \
    (ref) = (type)ntohll(*(type*)(stream)); \
    (stream) += sizeof(type); \
  } while(0)

  byte *pos = NULL;
  int error = NO_ERROR;
  SlotPageHeader header;

  assert(page_p != NULL && slot != NULL);

  getSlotPageHeader(page_p, &header);

  if (header.slot_count > slot_number) {
    pos = page_p->bytes + PAGE_SIZE - (slot_number + 1) * SLOT_SIZE;

    // read slot info
    slot->no = slot_number;

    READ_64BITS_FROM_STREAM(pos, uint64_t, slot->type);
    READ_64BITS_FROM_STREAM(pos, count_t, slot->length);
    READ_64BITS_FROM_STREAM(pos, offset_t, slot->start);
    READ_64BITS_FROM_STREAM(pos, offset_t, slot->end);
    READ_64BITS_FROM_STREAM(pos, number_t, slot->op_id);
    READ_64BITS_FROM_STREAM(pos, VolumeID, slot->next_pos.page_id.vol_id);
    READ_64BITS_FROM_STREAM(pos, id64_t, slot->next_pos.page_id.pg_id);
    READ_64BITS_FROM_STREAM(pos, number_t, slot->next_pos.slot_number);
  } else {
    error = ER_SLOT_PAGE_SLOT_OUT_OF_RANGE;
    SET_ERROR(error);
  }

  return error;

#undef READ_64BITS_FROM_STREAM
}

//static
int setSlot(Page *page_p, const number_t slot_number, const Slot *slot)
{
#define SAVE_UINT64_TO_STREAM(stream, ui64) \
  do { \
    tmp = htonll(ui64); \
    memcpy(stream, &tmp, sizeof(tmp)); \
    (stream) += sizeof(tmp); \
  } while(0)

  byte *pos = NULL;
  uint64_t tmp;
  SlotPageHeader header;

  int error = NO_ERROR;

  assert(page_p != NULL && slot != NULL);

  getSlotPageHeader(page_p, &header);

  if (header.slot_count > slot_number) {
    pos = page_p->bytes + PAGE_SIZE - (slot_number + 1) * SLOT_SIZE;

    // set slot info
    SAVE_UINT64_TO_STREAM(pos, slot->type);
    SAVE_UINT64_TO_STREAM(pos, slot->length);
    SAVE_UINT64_TO_STREAM(pos, slot->start);
    SAVE_UINT64_TO_STREAM(pos, slot->end);
    SAVE_UINT64_TO_STREAM(pos, slot->op_id);
    SAVE_UINT64_TO_STREAM(pos, slot->next_pos.page_id.vol_id);
    SAVE_UINT64_TO_STREAM(pos, slot->next_pos.page_id.pg_id);
    SAVE_UINT64_TO_STREAM(pos, slot->next_pos.slot_number);
  } else {
    error = ER_SLOT_PAGE_SLOT_OUT_OF_RANGE;
    SET_ERROR(error);
  }

  return error;

#undef SAVE_UINT64_TO_STREAM
}

//static
void getSlotPageHeader(Page *page_p, SlotPageHeader *header)
{
  assert(page_p != NULL && header != NULL);

  byte *pos = page_p->bytes;
  header->slot_count = ntohll(*(count_t*)pos);
}
//static
void setSlotPageHeader(Page *page_p, const SlotPageHeader *header)
{
  assert(page_p != NULL && header != NULL);

  count_t slot_count = htonll(header->slot_count);
  byte *pos = page_p->bytes;
  memcpy(pos, &slot_count, sizeof(slot_count));
}

//static
count_t getSlotPageLeftSize(const Page *page_p)
{
  count_t left_size = 0;
  Slot slot;
  SlotPageHeader header;

  assert (page_p != NULL);

  getSlotPageHeader(page_p, &header);

  count_t slot_count = header.slot_count;

  assert(slot_count > 0);



  left_size = PAGE_SIZE - (slot_count + 1) * SLOT_SIZE; // 1 for new slot

  (void)getSlot(page_p, slot_count - 1, &slot);
  left_size = left_size - slot.end - 1;

  return left_size;
}

void formatRawPage(byte *page_p)
{
  SlotPageHeader header;
  Slot slot;
  byte *pos = NULL;

  assert(page_p != NULL);

  //write the first slot with slot_header
  header.slot_count = 0;

  memcpy(page_p, htonll(header.slot_count), sizeof(header.slot_count));

  INIT_SLOT(&slot);
  slot.type = ALIVE;
  slot.length = SLOT_PAGE_HEADER_SIZE;
  slot.start = 0;
  slot.end = SLOT_PAGE_HEADER_SIZE - 1;
  slot.op_id = 0;

  pos = page_p + PAGE_SIZE - SLOT_SIZE;

  // set slot info
  SAVE_UINT64_TO_STREAM(pos, slot->type);
  SAVE_UINT64_TO_STREAM(pos, slot->length);
  SAVE_UINT64_TO_STREAM(pos, slot->start);
  SAVE_UINT64_TO_STREAM(pos, slot->end);
  SAVE_UINT64_TO_STREAM(pos, slot->op_id);
  SAVE_UINT64_TO_STREAM(pos, slot->next_pos.page_id.vol_id);
  SAVE_UINT64_TO_STREAM(pos, slot->next_pos.page_id.pg_id);
  SAVE_UINT64_TO_STREAM(pos, slot->next_pos.slot_number);
}

int getSlotPageRecord(const Page *page_p, const number_t slot_number, SlotPageRecord *record_p)
{
  int error = NO_ERROR;
  SlotPageHeader header;

  assert(page_p != NULL && record_p != NULL);

  getSlotPageHeader(page_p, &header);

  if (header.slot_count > slot_number) {
    error = getSlot(page_p, slot_number, &record_p->slot);
    if (error != NO_ERROR) {
      goto end;
    }

    record_p->length = record_p->slot.length;
    record_p->record_p = page_p->bytes + record_p->slot.start;
  } else {
    error = ER_SLOT_PAGE_SLOT_OUT_OF_RANGE;
    SET_ERROR(error);
  }

end:

  return error;
}

int setSlotPageRecord(Page *page_p, const number_t slot_number, const byte *record_p, const count_t record_len)
{
  int error = NO_ERROR;
  Slot slot;
  count_t size;
  byte *bytes = NULL;

  assert(page_p != NULL && record_p != NULL);

  error = getSlot(page_p, slot_number, &slot);
  if (error != NO_ERROR) {
    goto end;
  }

  size = slot.end - slot.start + 1;
  if (size >= record_len) {
    bytes = page_p->bytes + slot.start;

    memcpy(bytes, record_p, record_len);

    if (slot.length != record_len) {
      slot.length = record_len;

      // update slot info
      error = setSlot(page_p, slot_number, &slot);
      if (error != NO_ERROR) {
        goto end;
      }
    }
  } else {
    error = ER_SLOT_PAGE_NOT_ENOUGH_SPACE;
    SET_ERROR(error);
  }

end:

  return error;
}

/*
 * NOTE: updateSlotPageRecord may set other types of slot besides HOME
 */
int updateSlotPageRecord(Page *page_p, const number_t slot_number, const byte *record_p, const count_t record_len)
{
  int error = NO_ERROR;
  Slot slot;
  count_t size;
  byte *bytes = NULL;

  assert(page_p != NULL && record_p != NULL);

  // TODO : implement
  error = getSlot(page_p, slot_number, &slot);
  if (error != NO_ERROR) {
    goto end;
  }

  size = slot.end - slot.start + 1;
  if (size >= record_len) {
    bytes = page_p->bytes + slot.start;

    memcpy(bytes, record_p, record_len);

    if (slot.length != record_len) {
      slot.length = record_len;

      // update slot info
      error = setSlot(page_p, slot_number, &slot);
      if (error != NO_ERROR) {
        goto end;
      }
    }
  } else {
    // TODO : consider OVERFLOW,

    error = ER_SLOT_PAGE_NOT_ENOUGH_SPACE;
    SET_ERROR(error);
  }

end:

  return error;
}

int appendSlotPageRecord(Page *page_p, const byte *record_p, const count_t record_len)
{
  int error = NO_ERROR;
  Slot slot;
  count_t slot_count;
  number_t left_size = 0;
  offset_t record_start = 0;
  byte *bytes = NULL;
  count_t aligned_record_len;
  SlotPageHeader header;

  assert(page_p != NULL && record_p != NULL);

  getSlotPageHeader(&header);

  // calculate whether this page can hold the record
  slot_count = header.slot_count;

  assert(slot_count > 0);

  // TODO : consider reserve size
  aligned_record_len = ALIGN_DISK_DEFAULT(record_len);

  left_size = getSlotPageLeftSize(page_p);
  if (left_size >= aligned_record_len) {
    // get start copy position
    (void)getSlot(page_p, slot_count - 1, &slot);
    record_start = slot.end + 1;

    bytes = page_p->bytes;

    // write record
    memcpy(bytes + record_start, record_p, record_len);

    // write slot
    slot.no = slot_count;
    slot.type = ALIVE;
    slot.length = record_len;
    slot.start = record_start;
    slot.end = record_start + aligned_record_len - 1;

    (void)setSlot(page_p, slot_count, &slot);

    // update header
    header.slot_count += 1;
    (void)setSlotPageHeader(page_p, &header);
  } else {
    // TODO : consider OVERFLOW

    error = ER_SLOT_PAGE_NOT_ENOUGH_SPACE;
    SET_ERROR(error);
  }

end:

  return error;
}

int allocSlot(Page *page_p, const count_t size, Slot *slot, SlotType type)
{
  int error = NO_ERROR;
  count_t left_size = 0;

  assert(page_p != NULL && slot != NULL);

  INIT_SLOT(slot);

  size = ALIGN_DISK_DEFAULT(size);

  left_size = getSlotPageLeftSize(page_p);

  if (left_size >= size) {
    SlotPageHeader header;
    Slot last_slot;

    (void)getSlotPageHeader(page_p, &header);
    (void)getSlot(page_p, header.slot_count - 1, &last_slot);
    slot->no = header.slot_count;
    slot->type = type;
    slot->length = 0;
    slot->start = last_slot.end + 1;
    slot->end = last_slot.end + size;

    (void)setSlot(page_p, header.slot_count, slot);

    header.slot_count += 1;
    (void)setSlotPageHeader(page_p, &header);
  } else {
    error = ER_SLOT_PAGE_NOT_ENOUGH_SPACE;
    SET_ERROR(error);
  }

end:

  return error;
}

int allocOverflowSlot(Page *page_p, Slot *slot)
{
  int error = NO_ERROR;
  count_t size = 0;

  assert(page_p != NULL && slot != NULL);

  // TODO : calculate the overflow size

  error = allocSlot(page_p, size, slot, ALIVE_OVERFLOW);

  return error;
}

int setSlotType(Page *page_p, const number_t slot_number, const SlotType type)
{
  int error = NO_ERROR;
  Slot slot;

  assert(page_p != NULL);

  error = getSlot(page_p, slot_number, &slot);
  if (error != NO_ERROR) {
    goto end;
  }

  if (slot.type != type) {
    slot.type = type;
    error = setSlot(page_p, slot_number, &slot);
    if (error != NO_ERROR) {
      goto end;
    }
  }

end:

  return error;
}

int deleteSlot(Page *page_p, const number_t slot_number)
{
  int error = NO_ERROR;
  Slot slot;
  bool set_slot = false;

  assert(page_p != NULL && slot_number > 0);

  error = getSlot(page_p, slot_number, &slot);
  if (error != NO_ERROR) {
    goto end;
  }

  if (slot.type == ALIVE) {
    slot.type = DEAD;
    set_slot = true;
  } else if (slot.type == ALIVE_OVERFLOW) {
    slot.type = DEAD_OVERFLOW;
    set_slot = true;
  }

  if (set_slot) {
    error = setSlot(page_p, slot_number, &slot);
    if (error != NO_ERROR) {
      goto end;
    }
  }

end:

  return error;
}
