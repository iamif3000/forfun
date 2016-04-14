/*
 * file.h
 *
 *  Created on: Mar 10, 2016
 *      Author: duping
 */
#include "../common/id.h"
#include "page.h"

#ifndef FILE_H_
#define FILE_H_

typedef struct file File;
typedef enum file_type FileType;
typedef struct file_header FileHeader;

struct file {
  FileID id;
  FileHeader header;
  PageID start_page_id;
  PageID last_page_id;
  PageID current_page_id;
  Page *current_page;
};

enum file_type {
  FILE_TYPE_DATA,
  FILE_TYPE_BTREE,

  FILE_TYPE_LAST
};

struct file_header {
  FileType type;
  count_t page_count;
  count_t record_count;
};

int getFirstPage(const File *file, Page **page);
int getNextPage(const File *file, const Page *page, Page **next_page);
int getPrevPage(const File *file, const Page *page, Page **prev_page);
int getCurrentPage(const File *file, Page **page);
int getPageBySpace(const File *file, count_t space_at_least, Page **page);

#endif /* FILE_H_ */
