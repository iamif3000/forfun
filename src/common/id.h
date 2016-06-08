/*
 * id.h
 *
 *  Created on: Mar 10, 2016
 *      Author: duping
 */
#include "type.h"

#ifndef ID_H_
#define ID_H_

#define fileIDToStream slotIDToStream
#define streamToFileID streamToSlotID
#define bTIDToStream slotIDToStream
#define streamToBTID streamToSlotID

typedef id64_t VolumeID;
typedef struct slot_id SlotID;
typedef struct page_id PageID;
typedef struct slot_id FileID;
typedef struct slot_id BTID;

struct page_id {
  VolumeID vol_id;
  id64_t pg_id;
};

struct slot_id {
  PageID page_id;
  number_t slot_number;
};

inline static byte *pageIDToStream(byte *buf_p, PageID *page_id_p)
{
  id64_t id;

  assert(buf_p != NULL && page_id_p != NULL);

  id = htonll(page_id_p->vol_id);
  memcpy(buf_p, &id, sizeof(id));
  buf_p += sizeof(id);

  id = htonll(page_id_p->pg_id);
  memcpy(buf_p, &id, sizeof(id));
  buf_p += sizeof(id);

  return buf_p;
}

inline static byte *streamToPageID(byte *buf_p, PageID *page_id_p)
{
  id64_t id;

  assert(buf_p != NULL && page_id_p != NULL);

  memcpy(&id, buf_p, sizeof(id));
  buf_p += sizeof(id);
  id = ntohll(id);
  page_id_p->vol_id = id;

  memcpy(&id, buf_p, sizeof(id));
  buf_p += sizeof(id);
  id = ntohll(id);
  page_id_p->pg_id = id;

  return buf_p;
}

inline static byte *slotIDToStream(byte *buf_p, SlotID *slot_id_p)
{
  id64_t id;

  assert(buf_p != NULL && slot_id_p != NULL);

  id = htonll(slot_id_p->page_id.vol_id);
  memcpy(buf_p, &id, sizeof(id));
  buf_p += sizeof(id);

  id = htonll(slot_id_p->page_id.pg_id);
  memcpy(buf_p, &id, sizeof(id));
  buf_p += sizeof(id);

  id = htonll(slot_id_p->slot_number);
  memcpy(buf_p, &id, sizeof(id));
  buf_p += sizeof(id);

  return buf_p;
}

inline static byte *streamToSlotID(byte *buf_p, SlotID *slot_id_p)
{
  id64_t id;

  assert(buf_p != NULL && slot_id_p != NULL);

  memcpy(&id, buf_p, sizeof(id));
  buf_p += sizeof(id);
  slot_id_p->page_id->vol_id = ntohll(id);

  memcpy(&id, buf_p, sizeof(id));
  buf_p += sizeof(id);
  slot_id_p->page_id->pg_id = ntohll(id);

  memcpy(&id, buf_p, sizeof(id));
  buf_p += sizeof(id);
  slot_id_p->slot_number = ntohll(id);

  return buf_p;
}

#endif /* ID_H_ */
