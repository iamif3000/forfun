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

int initVolumeManager(const char *conf_file_p);
void destroyVolumeManager();

int registerCreatedVolume(Volume *vol);
Volume *getVolumeById(VolumeID id);


#endif /* VOLUME_MANAGER_H_ */
