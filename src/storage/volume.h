/*
 * volume.h
 *
 *  Created on: Mar 11, 2016
 *      Author: duping
 */

#include "../common/common.h"
#include "../base/base_string.h"

#ifndef VOLUME_H_
#define VOLUME_H_

#define LEAST_VOLUME_SIZE 0x400000 // 4M
#define INIT_VOLUME(v_p) \
  do { \
    (v_p)->id = 0; \
    (v_p)->fd = 0; \
    INIT_STRING(&(v_p)->path); \
  } while (0)

#define INIT_VOLUME_HEADER(h_p) \
  do { \
    (h_p)->volume_size = 0; \
    (h_p)->volume_type = PRIMARY_VOLUME; \
    (h_p)->page_size = 0; \
    (h_p)->page_count = 0; \
    (h_p)->page_map_start_page = 0; \
    (h_p)->page_map_end_page = 0; \
    (h_p)->current_alloc_page = 0; \
    (h_p)->allocatd_page_count = 0; \
    (h_p)->continuous_alloc = true; \
    (h_p)->file_tracker = { 0, 0 }; \
  } while(0)

#define NULL_VOLUMEID -1

#define IS_VOLUMEID_NULL(id) ((id) == NULL_VOLUMEID)

typedef struct volume Volume;
typedef struct volume_header VolumeHeader;
typedef enum volume_type VolumeType;

enum volume_type {
  PRIMARY_VOLUME,
  SECONDARY_VOLUME,
  LOG_VOLUME,
  TEMP_VOLUME,
  LAST_VOLUME_TYPE
};

struct volume {
  VolumeID id;
  int fd;
  String path;
};

struct volume_header {
  count_t volume_size;
  VolumeType volume_type;
  count_t page_size;
  count_t page_count;
  id64_t page_map_start_page;
  id64_t page_map_end_page;
  id64_t current_alloc_page;
  count_t allocatd_page_count;
  bool continuous_alloc;
  FileID file_tracker;
};

Volume *createVolume(const VolumeID id, const String *path_p, const count_t size);
Volume *loadVolume(const VolumeID id, const String *path_p);
int formatVolume(const Volume *volume_p, const VolumeType type);
int allocPages(Volume *volume_p, id64_t *pages_p, count_t pages_count);

int loadPage(const PageID page_id, byte *page_buf_p);
int savePage(const PageID page_id, const byte *page_buf_p);

#endif /* VOLUME_H_ */
