
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../common/common.h"
#include "../port/os_file.h"

#include "page.h"
#include "file.h"
#include "volume.h"

static int createVolumeFile(const String *path_p, count_t size);
static int setVolumeHeader(const Volume *vol_p, const VolumeHeader *vol_header_p);
static int getVolumeHeader(const Volume *vol_p, VolumeHeader *vol_header_p);

static byte *volumeHeaderToStream(byte *buf_p, VolumeHeader *header_p);
static byte *streamToVolumeHeader(byte *buf_p, VolumeHeader *header_p);

/*
 * static
 * return : < 0 error, > 0 fd
 */
int createVolumeFile(const String *path_p, count_t size)
{
  int error = NO_ERROR;

  assert(path_p != NULL);

  if (size < LEAST_VOLUME_SIZE) {
    size = LEAST_VOLUME_SIZE;
  }

  error = createFile(path_p->bytes, size);
  if (error < 0) {
    goto end;
  }

end:

  return error;
}

// static
int setVolumeHeader(const Volume *vol_p, const VolumeHeader *vol_header_p)
{
#define WRITE_UINT64(ui64) \
  do { \
    convert = (uint64_t)(ui64); \
    convert = htonll(convert); \
    error = writeFile(vol_p->fd, (byte*)&convert, sizeof(convert)); \
    if (error != NO_ERROR) { \
      goto end; \
    } \
  } while(0)

  int error = NO_ERROR;
  offset_t ofst = 0;
  uint64_t convert;

  assert(vol_p != NULL && vol_p->fd > 0 && vol_header_p != NULL);

  // move the start
  ofst = seekFile(vol_p->fd, 0, SEEK_SET);
  if (ofst < 0) {
    error = ER_OS_FILE_SEEK_FAIL;
    SET_ERROR(error);

    goto end;
  }

  // 8 bytes align
  WRITE_UINT64(vol_header_p->volume_size);
  WRITE_UINT64(vol_header_p->volume_type);
  WRITE_UINT64(vol_header_p->page_size);
  WRITE_UINT64(vol_header_p->page_count);
  WRITE_UINT64(vol_header_p->allocatd_page_count);
  WRITE_UINT64(vol_header_p->current_alloc_page);
  WRITE_UINT64(vol_header_p->file_tracker.vol_id);
  WRITE_UINT64(vol_header_p->file_tracker.pg_id);

  // other info

end:

  return error;

#undef WRITE_UINT64
}

//static
int getVolumeHeader(const Volume *vol_p, VolumeHeader *vol_header_p)
{
#define READ_UINT64(ui64) \
  do { \
    error = readFile(vol_p->fd, (byte*)&convert, sizeof(convert)); \
    if (error != NO_ERROR) { \
      goto end; \
    } \
    (ui64) = ntohll(convert); \
  } while(0)

  int error = NO_ERROR;
  offset_t ofst = 0;
  uint64_t convert;

  assert(vol_p != NULL && vol_p->fd > 0 && vol_header_p != NULL);

  // move the start
  ofst = seekFile(vol_p->fd, 0, SEEK_SET);
  if (ofst < 0) {
    error = ER_OS_FILE_SEEK_FAIL;
    SET_ERROR(error);

    goto end;
  }

  // 8 bytes align
  READ_UINT64(vol_header_p->volume_size);
  READ_UINT64(vol_header_p->volume_type);
  READ_UINT64(vol_header_p->page_size);
  READ_UINT64(vol_header_p->page_count);
  READ_UINT64(vol_header_p->allocatd_page_count);
  READ_UINT64(vol_header_p->current_alloc_page);
  READ_UINT64(vol_header_p->file_tracker.vol_id);
  READ_UINT64(vol_header_p->file_tracker.pg_id);

  // other info

end:

  return error;

#undef READ_UINT64
}

Volume *createVolume(const VolumeID id, const VolumeType type, const String *path_p, const count_t size)
{
  int error = NO_ERROR;
  Volume *vol_p = NULL;
  bool file_created = false;

  assert(path_p != NULL);

  vol_p = (Volume*)malloc(sizeof(Volume));
  if (vol_p == NULL) {
    error = ER_VOLUME_CREATE_VOLUME_FAIL;
    SET_ERROR(error);

    goto end;
  }

  INIT_VOLUME(vol_p);

  vol_p->id = id;
  vol_p->type = type;

  error = initString(&vol_p->path, path_p->size, NULL);
  if (error != NO_ERROR) {
    goto end;
  }

  error = copyString(&vol_p->path, path_p);
  if (error != NO_ERROR) {
    goto end;
  }

  // create file should be the last step
  error = createVolumeFile(path_p, size);
  if (error < 0) {
    goto end;
  }

  vol_p->fd = error;

end:

  if (error > 0) {
    error = NO_ERROR;
  }

  if (error != NO_ERROR && vol_p != NULL) {
    (void)destroyString(&vol_p->path);
    free(vol_p);

    vol_p = NULL;
  }

  return vol_p;
}

void destroyVolume(Volume *vol_p)
{
  assert(vol_p != NULL);

  if (vol_p->path.bytes != NULL) {
    (void)destroyString(&vol_p->path);
  }

  if (vol_p->fd > 0) {
    (void)closeFile(vol_p->fd);
  }

  free(vol_p);
}

Volume *loadVolume(const VolumeID id, const VolumeType type, const String *path_p)
{
  int error = NO_ERROR;
  Volume *vol_p = NULL;

  assert(!IS_VOLUMEID_NULL(id) && path_p != NULL);

  vol_p = (Volume*)malloc(sizeof(Volume));
  if (vol_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  memset(vol_p, 0, sizeof(Volume));

  error = initString(&vol_p->path, path_p->length + 1, path_p->bytes);
  if (error != NO_ERROR) {
    goto end;
  }

  error = openFile(path_p->bytes);
  if (error < 0) {
    goto end;
  }

  vol_p->fd = error;
  error = NO_ERROR;

  vol_p->id = id;
  vol_p->type = type;

end:

  if (error != NO_ERROR && vol_p != NULL) {
    destroyVolume(vol_p);
    vol_p = NULL;
  }

  return vol_p;
}

int formatVolume(const Volume *volume_p, VolumeType type)
{
  int error = NO_ERROR;
  offset_t ofst = 0;
  count_t page_count = 0;
  id64_t next_page = 0;
  //count_t page_map_page_count = 0;
  VolumeHeader header;

  assert(volume_p != NULL && volume_p->fd > 0);

  INIT_VOLUME_HEADER(&header);

  // move to the end of file
  ofst = seekFile(volume_p->fd, 0, SEEK_END);
  if (ofst < 0) {
    error = (int)ofst;
    goto end;
  }

  assert(ofst % PAGE_SIZE == 0);

  // calculate the header
  page_count = ofst / PAGE_SIZE;

  header.volume_size = ofst;
  header.volume_type = type;

  header.page_size = PAGE_SIZE;
  header.page_count = page_count;

  //page_map_page_count = ALIGN_BYTES((ALIGN_BYTES(page_count, 8) >> 3), PAGE_SIZE) / PAGE_SIZE;

  // move to the start of file
  ofst = seekFile(volume_p->fd, 0, SEEK_SET);
  if (ofst < 0) {
    error = (int)ofst;
    goto end;
  }

  // init the free page list
  for (count_t i = 0; i < page_count; ++i) {
    if (i < page_count - 1) {
      next_page = htonll(i + 1);
    } else {
      next_page = htonll(NULL_PAGEIDX);
    }

    error = writeFileToOffset(volume_p->fd, (byte*)&next_page, sizeof(id64_t), (i * PAGE_SIZE));
    if (error < 0) {
      goto end;
    }
  }

  {
#define BUF_SIZE 1024
    Page page;
    PageID next_id, prev_id;
    byte buf[BUF_SIZE];
    byte *base_p = buf;
    byte *buf_p = buf;
    byte *new_buf_len = 0;
    byte *new_buf_p = NULL;
    File *file_p = NULL;

    if (type != PRIMARY_VOLUME) {
      // page0 for volume header
      header.current_alloc_page = 1;
      header.allocatd_page_count = 1;

    } else {
      // page0 for volume header, page1 for file_tracker
      // save page1
      header.current_alloc_page = 2;
      header.allocatd_page_count = 2;

      SET_PAGEID(&header.file_tracker, 0, 1);

      // the first file is file_tracker
      file_p = (File*)malloc(sizeof(File));
      if (file_p == NULL) {
        error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
        SET_ERROR(error);
        goto block_end;
      }

      initFile(file_p);

      SET_PAGEID(&file_p->current_page_id, 0, 1);
      SET_PAGEID(&file_p->start_page_id, 0, 1);
      SET_PAGEID(&file_p->end_page_id, 0, 1);
      SET_FILEID(&file_p->id, 0, 1, START_SLOT_IN_DATA_PAGE);
      file_p->header.type = FILE_TYPE_DATA;
      file_p->header.page_count = 1;
      file_p->header.record_count = 1; // file_tracker it-self

      // save to disk

      formatRawPage(&page.bytes[0]);

      SET_PAGEID_NULL(&prev_id);
      SET_PAGEID_NULL(&next_id);

      buf_p = pageIDToStream(buf_p, &prev_id);
      buf_p = pageIDToStream(buf_p, &next_id);

      // slot1 is the file linker part
      error = appendSlotPageRecord(&page, base_p, buf_p - base_p);
      if (error != NO_ERROR) {
        goto block_end;
      }

      // slot2 is the data part start
      buf_p = base_p;

      count_t file_size = getFileStreamSize(file_p);
      if (file_size > BUF_SIZE) {
        new_buf_len = sizeof(byte) * file_size;
        new_buf_p = (byte*)malloc(new_buf_len);
        if (new_buf_p == NULL) {
          error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
          SET_ERROR(error);
          goto block_end;
        }

        base_p = new_buf_p;
        buf_p = new_buf_p;
      }

      buf_p = fileToStream(buf_p, file_p);

      error = appendSlotPageRecord(&page, base_p, buf_p - base_p);
      if (error != NO_ERROR) {
        goto block_end;
      }

      // write page to
      error = savePageToVolume(volume_p, 1, page.bytes);
      if (error != NO_ERROR) {
        goto block_end;
      }
    }

    // TODO: save page0
    buf_p = base_p;

    ;

block_end:

    if (new_buf_p != NULL) {
      free(new_buf_p);
    }

    if (file_p != NULL) {
      clearFile(file_p);
      free(file_p);
      free(file_p);
    }

    if (error != NO_ERROR) {
      goto end;
    }

#undef BUF_SIZE
  }



  error = NO_ERROR;

end:

  return error;
}

int allocPagesFromVolume(Volume *volume_p, id64_t *pages_p, const count_t pages_count)
{
  int error = NO_ERROR;
  VolumeHeader header;
  count_t i = 0;
  id64_t current_page_idx = 0;
  id64_t next_page_idx = 0;

  assert(volume_p != NULL && pages_p != NULL && pages_count > 0);

  error = getVolumeHeader(volume_p, &header);
  if (error != NO_ERROR) {
    goto end;
  }

  if (header.allocatd_page_count + pages_count > header.page_count) {
    error = ER_VOLUME_NOT_ENOUGH_PAGE;
    SET_ERROR(error);

    goto end;
  }

  current_page_idx = header.current_alloc_page;

  i = 0;

  do {
    if (current_page_idx == NULL_PAGEIDX) {
      error = ER_VOLUME_NOT_ENOUGH_PAGE;
      SET_ERROR(error);

      goto end;
    }

    error = readFileFromOffset(volume_p->fd, (byte*)&next_page_idx, sizeof(id64_t), (current_page_idx * PAGE_SIZE));
    if (error < 0) {
      goto end;
    }

    next_page_idx = ntohll(next_page_idx);

    pages_p[i] = current_page_idx;
    current_page_idx = next_page_idx;

  } while (++i < pages_count);

  // update header
  header.allocatd_page_count += pages_count;
  header.current_alloc_page = next_page_idx;

  error = setVolumeHeader(volume_p, &header);
  if (error != NO_ERROR) {
    goto end;
  }

  if (error > 0) {
    error = NO_ERROR;
  }

end:

  return error;
}

int freePagesToVolume(Volume *volume_p, const id64_t *pages_p, const count_t pages_count)
{
  int error = NO_ERROR;
  VolumeHeader header;
  id64_t next_page_idx = 0;
  count_t i = 0;

  assert(volume_p != NULL && pages_p != NULL && pages_count > 0);

  error = getVolumeHeader(volume_p, &header);
  if (error != NO_ERROR) {
    goto end;
  }

  next_page_idx = header.current_alloc_page;

  i = 0;

  do {
    next_page_idx = htonll(next_page_idx);

    assert(pages_p[i] != NULL_PAGEIDX);

    error = writeFileToOffset(volume_p->fd, (byte*)&next_page_idx, sizeof(id64_t), (pages_p[i] * PAGE_SIZE));
    if (error < 0) {
      goto end;
    }

    next_page_idx = pages_p[i];
  } while (++i < pages_count);

  // update header
  header.allocatd_page_count += pages_count;
  header.current_alloc_page = next_page_idx;

  error = setVolumeHeader(volume_p, &header);
  if (error != NO_ERROR) {
    goto end;
  }

  if (error > 0) {
    error = NO_ERROR;
  }

end:

  return error;
}

int loadPageFromVolume(Volume *volume_p, const id64_t page_id, byte *page_buf_p)
{
  int error = NO_ERROR;

  assert(volume_p != NULL && page_id != NULL_PAGEIDX && page_buf_p != NULL);

  error = readFileFromOffset(volume_p->fd, page_buf_p, PAGE_SIZE, (page_id * PAGE_SIZE));

  return error;
}

int savePageToVolume(Volume *volume_p, const id64_t page_id, const byte *page_buf_p)
{
  int error = NO_ERROR;

  assert(volume_p != NULL && page_id != NULL_PAGEIDX && page_buf_p != NULL);

  error = writeFileToOffset(volume_p->fd, page_buf_p, PAGE_SIZE, (page_id * PAGE_SIZE));

  return error;
}
