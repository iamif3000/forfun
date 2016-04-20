/*
 * volume_manager_test.c
 *
 *  Created on: Apr 20, 2016
 *      Author: duping
 */

#include <stdio.h>
#include <string.h>

#include "../../common/error.h"

#include "../../storage/volume.h"
#include "../../storage/volume_manager.h"

int main(void)
{
  int error = NO_ERROR;
  bool test_fail = false;

  VolumeManager *manager_p = NULL;
  VolumeList *voll_p = NULL;

  error = initVolumeManager("./volume.conf");
  if (error != NO_ERROR) {
    goto end;
  }

  manager_p = getVolumeManager();

  assert(manager_p != NULL);

  // data
  voll_p = manager_p->data_vol.next_p;
  if (voll_p == NULL
      || voll_p->vol_p->type != SECONDARY_VOLUME
      || strncmp(voll_p->vol_p->path.bytes, "logic_volume", voll_p->vol_p->path.length + 1) != 0) {
    test_fail = true;
  }

  if (!test_fail) {
    voll_p = voll_p->next_p;
    if (voll_p == NULL
        || voll_p->vol_p->type != PRIMARY_VOLUME
        || strncmp(voll_p->vol_p->path.bytes, "primary_volume", voll_p->vol_p->path.length + 1) != 0) {
      test_fail = true;
    }
  }

  if (!test_fail) {
    voll_p = manager_p->log_vol.next_p;
    if (voll_p == NULL
        || voll_p->vol_p->type != LOG_VOLUME
        || strncmp(voll_p->vol_p->path.bytes, "log_volume", voll_p->vol_p->path.length + 1) != 0) {
      test_fail = true;
    }
  }

  if (!test_fail) {
    voll_p = manager_p->tmp_vol.next_p;
    if (voll_p == NULL
        || voll_p->vol_p->type != TEMP_VOLUME
        || strncmp(voll_p->vol_p->path.bytes, "temp_volume", voll_p->vol_p->path.length + 1) != 0) {
      test_fail = true;
    }
  }

end:

  if (error != NO_ERROR) {
    test_fail = true;
  }

  if (test_fail) {
    printf("volume_manager_test fail.\n");
  } else {
    printf("volume_manager_test succeed.\n");
  }

  destroyVolumeManager();

  if (error != NO_ERROR) {
    return -1;
  }

  return 0;
}
