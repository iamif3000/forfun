/*
 * data_page.h
 *
 *  Created on: Mar 11, 2016
 *      Author: duping
 */

#include "slot_page.h"

#ifndef FILE_PAGE_H_
#define FILE_PAGE_H_

typedef struct file_page FilePage;
typedef struct file_page_header FilePageHeader;

struct file_page {
  FilePageHeader header; // slot[0]
  SlotPage slot_page;
};

struct file_page_header {
  PageID prev_page_id;
  PageID next_page_id;
  FileID file_id;
};

#endif /* DATA_PAGE_H_ */
