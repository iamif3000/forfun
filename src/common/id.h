/*
 * id.h
 *
 *  Created on: Mar 10, 2016
 *      Author: duping
 */
#include "type.h"

#ifndef ID_H_
#define ID_H_

typedef id64_t VolumeID;
typedef struct page_id PageID;
typedef struct page_id FileID;

struct page_id {
  VolumeID vol_id;
  id64_t pg_id;
};

#endif /* ID_H_ */
