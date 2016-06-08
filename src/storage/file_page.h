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
typedef enum btree_page_type BtreePageType;

enum btree_page_type {
  PAGE_NONLEAF,
  PAGE_LEAF
};

struct file_page {
  SlotPage slot_page;
};

struct file_page_header { // slot[1]
  PageID prev_page_id;
  PageID next_page_id;
  FileID file_id;
};

struct btree_page_header { // slot[2]
  BtreePageType type;
};

#endif /* DATA_PAGE_H_ */
