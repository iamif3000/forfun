/*
 * file.c
 *
 *  Created on: Mar 10, 2016
 *      Author: duping
 */

#include "../common/common.h"

#include "file.h"
#include "slot_page.h"
#include "volume_manager.h"

#define MIN_PREALLOC_PAGES 1
#define DEFAULT_PAGES 20

static File *createFileInternal(String *name_p, int prealloc_pages, FileType type);
static byte *fileHeaderToStream(byte *buf_p, FileHeader *header_p);
static byte *streamToFileHeader(byte *buf_p, FileHeader *header_p);

/*
 * static
 */
File *createFileInternal(String *name_p, int prealloc_pages, FileType type)
{
  int error = NO_ERROR;
  File *file_p = NULL;
  PageID pages[DEFAULT_PAGES];
  PageID pages_p = pages;
  bool page_alloced = false;

  assert(name_p != NULL);

  if (prealloc_pages < MIN_PREALLOC_PAGES) {
    prealloc_pages = MIN_PREALLOC_PAGES;
  }

  if (prealloc_pages > DEFAULT_PAGES) {
    pages_p = (PageID*)malloc(sizeof(PageID) * prealloc_pages);
    if (pages_p == NULL) {
      error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
      SET_ERROR(error);
      goto end;
    }
  }

  // alloc pages and then add to file-tracker
  error = allocPages(pages_p, prealloc_pages, PRIMARY_VOLUME);
  if (error != NO_ERROR) {
    SET_ERROR(error);
    goto end;
  }

  page_alloced = true;

  file_p = (File*)malloc(sizeof(File));
  if (file_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);
    goto end;
  }

  initFile(file_p);

  // link the pages
  byte page_buf[PAGE_SIZE];

  for (int i = 0; i < prealloc_pages - 1; ++i) {
    error = loadPage(pages_p[i], &page_buf[0]);
    if (error != NO_ERROR) {
      SET_ERROR(error);
      goto end;
    }

    *(id64_t*)&page_buf[0] = htonll(pages_p[i + 1].vol_id);
    *((id64_t*)&page_buf[0] + 1) = htonll(pages_p[i + 1].pg_id);

    error = savePage(pages_p[i], &page_buf[0]);
    if (error != NO_ERROR) {
      SET_ERROR(error);
      goto end;
    }
  }

  error = loadPage(pages_p[prealloc_pages - 1], &page_buf[0]);
  if (error != NO_ERROR) {
    SET_ERROR(error);
    goto end;
  }

  *(id64_t*)&page_buf[0] = htonll(NULL_VOLUMEID);
  *((id64_t*)&page_buf[0] + 1) = htonll(NULL_PAGEIDX);

  error = savePage(pages_p[prealloc_pages - 1], &page_buf[0]);
  if (error != NO_ERROR) {
    SET_ERROR(error);
    goto end;
  }

  SET_PAGEID(&file_p->prealloc_start_page_id, pages_p[0].vol_id, pages_p[0].pg_id);
  SET_PAGEID(&file_p->start_page_id, pages_p[0].vol_id, pages_p[0].pg_id);
  SET_PAGEID(&file_p->end_page_id, pages_p[prealloc_pages - 1].vol_id, pages_p[prealloc_pages - 1].pg_id);
  SET_PAGEID(&file_p->current_page_id, pages_p[0].vol_id, pages_p[0].pg_id);

  file_p->header.type = type;

  // TODO: add this file to file-tracker

end:

  if (error != NO_ERROR) {
    if (page_alloced) {
      (void)freePages(pages_p, prealloc_pages);
    }

    if (file_p != NULL) {
      free(file_p);
      file_p = NULL;
    }
  }

  if (pages_p != NULL && pages_p != &pages[0]) {
    free(pages_p);
  }

  return file_p;
}

/*
 * static
 */
byte *fileHeaderToStream(byte *buf_p, FileHeader *header_p)
{
  uint64_t u64;

  assert(buf_p != NULL && header_p != NULL);

  u64 = htonll(header_p->type);
  memcpy(buf_p, &u64, sizeof(u64));
  buf_p += sizeof(u64);

  u64 = htonll(header_p->page_count);
  memcpy(buf_p, &u64, sizeof(u64));
  buf_p += sizeof(u64);

  u64 = htonll(header_p->record_count);
  memcpy(buf_p, &u64, sizeof(u64));
  buf_p += sizeof(u64);

  return buf_p;
}

/*
 * static
 */
byte *streamToFileHeader(byte *buf_p, FileHeader *header_p)
{
  uint64_t u64;

  assert(buf_p != NULL && header_p != NULL);

  memcpy(&u64, buf_p, sizeof(u64));
  buf_p += sizeof(u64);
  header_p->type = ntohll(u64);

  memcpy(&u64, buf_p, sizeof(u64));
  buf_p += sizeof(u64);
  header_p->page_count = ntohll(u64);

  memcpy(&u64, buf_p, sizeof(u64));
  buf_p += sizeof(u64);
  header_p->record_count = ntohll(u64);

  return buf_p;
}

void initFile(File *file_p)
{
  assert(file_p != NULL);

  SET_PAGEID_NULL(&file_p->start_page_id);
  SET_PAGEID_NULL(&file_p->end_page_id);
  SET_PAGEID_NULL(&file_p->current_page_id); // root_page_id
  SET_PAGEID_NULL(&file_p->start_overflow_page_id);
  SET_PAGEID_NULL(&file_p->prealloc_start_page_id);

  file_p->header.page_count = 0;
  file_p->header.record_count = 0; // key_count
  file_p->header.type = FILE_TYPE_DATA;

  SET_FILEID_NULL(&file_p->id);
}

byte *fileToStream(byte *buf_p, File *file_p)
{
  assert(buf_p != NULL && file_p != NULL);

  buf_p = fileIDToStream(buf_p, &file_p->id);
  buf_p = fileHeaderToStream(buf_p, &file_p->header);
  buf_p = pageIDToStream(buf_p, &file_p->start_page_id);
  buf_p = pageIDToStream(buf_p, &file_p->end_page_id);
  buf_p = pageIDToStream(buf_p, &file_p->prealloc_start_page_id);
  buf_p = pageIDToStream(buf_p, &file_p->start_overflow_page_id);
  buf_p = pageIDToStream(buf_p, &file_p->current_page_id);

  return buf_p;
}

byte *streamToFile(byte *buf_p, File *file_p)
{
  assert(buf_p != NULL && file_p != NULL);

  buf_p = streamToFileID(buf_p, &file_p->id);
  buf_p = streamToFileHeader(buf_p, &file_p->header);
  buf_p = streamToPageID(buf_p, &file_p->start_page_id);
  buf_p = streamToPageID(buf_p, &file_p->end_page_id);
  buf_p = streamToPageID(buf_p, &file_p->prealloc_start_page_id);
  buf_p = streamToPageID(buf_p, &file_p->start_overflow_page_id);
  buf_p = streamToPageID(buf_p, &file_p->current_page_id);

  return buf_p;
}

File *createDataFile(String *name_p, int prealloc_pages)
{
  File *file_p = NULL;

  assert(name_p != NULL);

  return file_p;
}

File *createBtreeFile(String *name_p, int prealloc_pages)
{
  File *file_p = NULL;

  assert(name_p != NULL);

  if (prealloc_pages < MIN_PREALLOC_PAGES) {
    prealloc_pages = MIN_PREALLOC_PAGES;
  }

  // TODO: alloc pages and then add to file-tracker

  return file_p;
}
