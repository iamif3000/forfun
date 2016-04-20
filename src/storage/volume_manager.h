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

int registerCreatedVolume(Volume *vol);
Volume *getVolumeById(VolumeID id);

// for test only
VolumeManager *getVolumeManager();

#endif /* VOLUME_MANAGER_H_ */
