/*
 * volume_manager.c
 *
 *  Created on: Apr 15, 2016
 *      Author: duping
 */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "../port/os_file.h"

#include "volume_manager.h"

typedef enum parse_status ParseStatus;

enum parse_status {
  P_VOLUME_ID_START,
  P_VOLUME_ID_END,
  P_VOLUME_TYPE_START,
  P_VOLUME_TYPE_END,
  P_VOLUME_PATH_START,
  P_VOLUME_PATH_END,
  P_COMMENT
};

static VolumeManager *manager_p = NULL;

static void initVolumeList(VolumeList *node_p);
static int parseVolumeConfFile(const int fd);
static int addVolumeToManager(Volume *vol_p);

/*
 * static
 */
void initVolumeList(VolumeList *node_p)
{
  assert(node_p != NULL);

  node_p->next_p = NULL;
  node_p->vol_p = NULL;
}

/*
 * static
 * Format : volume_id volume_type "volume_path"
 * # means comment
 */
int parseVolumeConfFile(const int fd)
{
#define BUF_SIZE 1024

  ssize_t error = NO_ERROR, buf_size = 0;
  byte buf[BUF_SIZE];
  byte c;
  int i = 0, field_len = 0;
  ParseStatus status = P_VOLUME_ID_START;

  VolumeID id = 0;
  VolumeType type = 0;
  String path;
  Volume *volume_p = NULL;

  initString(&path, 128, NULL);

  assert(manager_p != NULL && fd > 0);

  do {
    error = readFile(fd, buf, BUF_SIZE);
    if (error < 0) {
      goto end;
    } else if (error == NO_ERROR) {
      // end of file
      break;
    }

    i = 0;
    buf_size = error;

    do {
      c = buf[i];
      switch (c) {
      case '#': // comment
        if (status != P_COMMENT) {
          if (status != P_VOLUME_ID_START && status != P_VOLUME_PATH_END) {
            error = ER_VM_CONF_ERROR;
            SET_ERROR(error);

            goto end;
          }

          status = P_COMMENT;
        }

        break;
      case '\n':
        if (status != P_COMMENT && status != P_VOLUME_PATH_END) {
            error = ER_VM_CONF_ERROR;
            SET_ERROR(error);

            goto end;
        }

        status = P_VOLUME_ID_START;
        field_len = 0;

        break;
      case ' ':
      case '\t':
        switch (status) {
        case P_VOLUME_ID_START:
          if (field_len < 1) {
            error = ER_VM_CONF_ERROR;
            SET_ERROR(error);

            goto end;
          }

          status = P_VOLUME_ID_END;

          break;
        case P_VOLUME_TYPE_START:
          if (field_len < 1 || type < PRIMARY_VOLUME || type >= LAST_VOLUME_TYPE) {
            error = ER_VM_CONF_ERROR;
            SET_ERROR(error);

            goto end;
          }

          status = P_VOLUME_TYPE_END;

          break;
        default:
          // do nothing
          break;
        }

        break;
      case '"':
        if (status == P_VOLUME_TYPE_END) {
          status = P_VOLUME_PATH_START;
          field_len = 0;
        } else if (status == P_VOLUME_PATH_START) {
          if (field_len < 1) {
            error = ER_VM_CONF_ERROR;
            SET_ERROR(error);

            goto end;
          }

          // one line parse end, load volume
          volume_p = loadVolume(id, type, &path);
          if (volume_p == NULL) {
            error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
            SET_ERROR(error);

            goto end;
          }

          error = addVolumeToManager(volume_p);
          if (error != NO_ERROR) {
            goto end;
          }

          //reset parse vars
          id = 0;
          type = 0;
          resetString(&path);

          status = P_VOLUME_PATH_END;
        } else {
          error = ER_VM_CONF_ERROR;
          SET_ERROR(error);

          goto end;
        }

        break;
      default:
        switch (status) {
        case P_VOLUME_ID_START:
          if (c >= '0' && c <= '9') {
            id = id * 10 + c - '0';
            ++field_len;
          } else {
            error = ER_VM_CONF_ERROR;
            SET_ERROR(error);

            goto end;
          }

          break;
        case P_VOLUME_ID_END:
          status = P_VOLUME_TYPE_START;
          field_len = 0;
        case P_VOLUME_TYPE_START:
          if (c >= '0' && c <= '9') {
            type = type * 10 + c - '0';
            ++field_len;
          } else {
            error = ER_VM_CONF_ERROR;
            SET_ERROR(error);

            goto end;
          }

          break;
        case P_VOLUME_PATH_START:
          error = appendByte(&path, c);
          if (error != NO_ERROR) {
            goto end;
          }

          ++field_len;

          break;
        case P_COMMENT:
          break;
        default:
          error = ER_VM_CONF_ERROR;
          SET_ERROR(error);

          goto end;
        }

        break;
      }
    } while(++i < buf_size);
  } while(true);

  if (status != P_VOLUME_ID_START && status != P_VOLUME_PATH_END && status != P_COMMENT) {
    error = ER_VM_CONF_ERROR;
    SET_ERROR(error);

    goto end;
  }

end:

  destroyString(&path);

  if (error > 0) {
    error = NO_ERROR;
  }

  return (int)error;

#undef BUF_SIZE
}

/*
 * static
 */
int addVolumeToManager(Volume *vol_p)
{
  int error = NO_ERROR;
  VolumeList *list_p = NULL;
  VolumeList *head_p = NULL;

  assert(manager_p != NULL && vol_p != NULL
      && vol_p->type >= PRIMARY_VOLUME && vol_p->type <  LAST_VOLUME_TYPE);

  list_p = (VolumeList*)malloc(sizeof(VolumeList));
  if (list_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  list_p->vol_p = vol_p;
  list_p->next_p = NULL;

  if (vol_p->type == PRIMARY_VOLUME || vol_p->type == SECONDARY_VOLUME) {
    head_p = &manager_p->data_vol;
  } else if (vol_p->type == LOG_VOLUME) {
    head_p = &manager_p->log_vol;
  } else {
    head_p = &manager_p->tmp_vol;
  }

  list_p->next_p = head_p->next_p;
  head_p->next_p = list_p;

end:

  return error;
}

VolumeManager *getVolumeManager()
{
  return manager_p;
}

int initVolumeManager(const char *conf_file_p)
{
  int error = NO_ERROR;

  assert(conf_file_p != NULL);

  if (manager_p == NULL) {
    manager_p = (VolumeManager*)malloc(sizeof(VolumeManager));
    if (manager_p == NULL) {
      error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
      SET_ERROR(error);

      goto end;
    }

    memset(manager_p, 0, sizeof(VolumeManager));

    // init manager_p
    error = openFile(conf_file_p);
    if (error < 0) {
      goto end;
    }

    manager_p->conf_file_fd = error;
    error = NO_ERROR;

    error = initString(&manager_p->conf_file_path, strlen(conf_file_p), conf_file_p);
    if (error != NO_ERROR) {
      goto end;
    }

    manager_p->total_count = 0;
    manager_p->data_count = 0;
    manager_p->log_count = 0;
    manager_p->tmp_count = 0;
    initVolumeList(&manager_p->data_vol);
    initVolumeList(&manager_p->log_vol);
    initVolumeList(&manager_p->tmp_vol);

    // load the volume info from conf file
    error = parseVolumeConfFile(manager_p->conf_file_fd);
    if (error != NO_ERROR) {
      goto end;
    }
  }

end:

  if (error != NO_ERROR && manager_p != NULL) {
    destroyVolumeManager(manager_p);
    manager_p = NULL;
  }

  return error;
}

void destroyVolumeManager()
{
  VolumeList *cur_p = NULL, *next_p = NULL;
  VolumeList *list[3];

  if (manager_p != NULL) {
    list[0] = manager_p->data_vol.next_p;
    list[1] = manager_p->log_vol.next_p;
    list[2] = manager_p->tmp_vol.next_p;

    (void)destroyString(&manager_p->conf_file_path);

    if (manager_p->conf_file_fd > 0) {
      closeFile(manager_p->conf_file_fd);
      manager_p->conf_file_fd = 0;
    }

    for (int i = 0; i < sizeof(list)/sizeof(list[0]); ++i) {
      cur_p = list[i];
      while (cur_p) {
        next_p = cur_p->next_p;

        // free list and volume
        destroyVolume(cur_p->vol_p);
        free(cur_p);

        cur_p = next_p;
      }
    }

    free(manager_p);
  }
}

int registerCreatedVolume(Volume *vol)
{
  int error = NO_ERROR;

  assert(vol != NULL);

  return error;
}

Volume *getVolumeById(VolumeID id)
{
  Volume *vol_p = NULL;

  assert(!IS_VOLUMEID_NULL(id));

  return vol_p;
}
