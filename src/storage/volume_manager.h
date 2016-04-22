/*
 * volume_manager.h
 *
 *  Created on: Apr 15, 2016
 *      Author: duping
 */

#include "../common/common.h"

#include "../base/base_string.h"

#include "volume.h"

#ifndef VOLUME_MANAGER_H_
#define VOLUME_MANAGER_H_

#define destroyVolumeManagerAndSetNull(mgr_p) \
  do { \
    destroyVolumeManager(mgr_p); \
    mgr_p = NULL; \
  } while(0)

typedef struct volume_list VolumeList;
typedef struct volume_manager VolumeManager;

struct volume_list {
  VolumeList *next_p;
  Volume *vol_p;
};

struct volume_manager {
  String conf_file_path;
  int conf_file_fd;
  count_t total_count;
  count_t data_count;
  count_t log_count;
  count_t tmp_count;
  VolumeList data_vol;  // sentinel
  VolumeList log_vol;   // sentinel
  VolumeList tmp_vol;  // sentinel
};

int initVolumeManager(const char *conf_file_p);
void destroyVolumeManager();

Volume *createNewVolume(VolumeType type);
int registerCreatedVolume(Volume *vol);
Volume *getVolumeById(VolumeID id);

int allocPages(PageID *pages_p, count_t pages_count, VolumeType type);
int freePages(PageID *pages_p, count_t pages_count);

int loadPage(const PageID page_id, byte *page_buf_p);
int savePage(const PageID page_id, const byte *page_buf_p);

// for test only
VolumeManager *getVolumeManager();

#endif /* VOLUME_MANAGER_H_ */
