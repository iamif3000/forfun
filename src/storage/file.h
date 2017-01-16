/*
 * file.h
 *
 *  Created on: Mar 10, 2016
 *      Author: duping
 */
#include "../common/id.h"

#include "../base/base_string.h"

#include "page.h"

#ifndef FILE_H_
#define FILE_H_

#define START_SLOT_IN_DATA_PAGE 2
#define START_SLOT_IN_BTREE_PAGE 3

#define SET_FILEID(id_p, volume_id, page_idx, slot_id) \
  do { \
    (id_p)->page_id.vol_id = (volume_id); \
    (id_p)->page_id.pg_id = (page_idx); \
    (id_p)->slot_number = (slot_id); \
  } while(0)

#define SET_FILEID_NULL(id_p) SET_FILEID(id_p, NULL_VOLUMEID, NULL_PAGEIDX, 0)

typedef struct file File;
typedef enum file_type FileType;
typedef struct file_header FileHeader;

enum file_type {
  FILE_TYPE_DATA,
  FILE_TYPE_BTREE,

  FILE_TYPE_LAST
};

struct file {
  FileID id;
  FileHeader header;

  PageID start_page_id;             // content
  PageID end_page_id;               // content
  PageID prealloc_start_page_id;    // prealloc empty
  PageID start_overflow_page_id;    // content

  PageID current_page_id;           // content

  String name;
};

struct file_header {
  FileType type;
  count_t page_count;
  count_t record_count;             // slots which are not vanished.
};

// for format volume
void initFile(File *file_p);
void clearFile(File *file_p);
count_t getFileStreamSize(File *file_p);
byte *fileToStream(byte *buf_p, File *file_p);
byte *streamToFile(byte *buf_p, File *file_p, int *error);

File *createDataFile(String *name_p, int prealloc_pages);
File *createBtreeFile(String *name_p, int prealloc_pages);

#endif /* FILE_H_ */
