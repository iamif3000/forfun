#include "../common/common.h"
#include "../port/os_file.h"
#include "page.h"
#include "volume.h"

static int createVolumeFile(const String *path_p, const count_t size);
static int setVolumeHeader(const Volume *vol_p, const VolumeHeader *vol_header_p);
static int getVolumeHeader(const Volume *vol_p, VolumeHeader *vol_header_p);

/*
 * static
 * return : < 0 error, > 0 fd
 */
int createVolumeFile(const String *path_p, const count_t size)
{
  int error = NO_ERROR;

  assert(path_p != NULL);

  if (size < LEAST_VOLUME_SIZE) {
    size = LEAST_VOLUME_SIZE;
  }

  error = createFile(path_p, size);
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
    error = writeFile(vol_p->fd, &convert, sizeof(convert)); \
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
    goto end;
  }

  // 8 bytes align
  WRITE_UINT64(vol_header_p->volume_size);
  WRITE_UINT64(vol_header_p->volume_type);
  WRITE_UINT64(vol_header_p->page_size);
  WRITE_UINT64(vol_header_p->page_count);
  WRITE_UINT64(vol_header_p->page_map_start_page);
  WRITE_UINT64(vol_header_p->page_map_end_page);
  WRITE_UINT64(vol_header_p->page_size);
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
    error = readFile(vol_p->fd, &convert, sizeof(convert)); \
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
    goto end;
  }

  // 8 bytes align
  READ_UINT64(vol_header_p->volume_size);
  READ_UINT64(vol_header_p->volume_type);
  READ_UINT64(vol_header_p->page_size);
  READ_UINT64(vol_header_p->page_count);
  READ_UINT64(vol_header_p->page_map_start_page);
  READ_UINT64(vol_header_p->page_map_end_page);
  READ_UINT64(vol_header_p->page_size);
  READ_UINT64(vol_header_p->current_alloc_page);
  READ_UINT64(vol_header_p->file_tracker.vol_id);
  READ_UINT64(vol_header_p->file_tracker.pg_id);

  // other info

end:

  return error;

#undef READ_UINT64
}

int createVolume(int id, const String *path_p, const count_t size, Volume **volume_p)
{
  int error = NO_ERROR;
  Volume *vol_p = NULL;
  bool file_created = false;

  assert(path_p != NULL && volume_p != NULL);

  vol_p = (Volume*)malloc(sizeof(Volume));
  if (vol_p == NULL) {
    error = ER_VOLUME_CREATE_VOLUME_FAIL;
    goto end;
  }

  INIT_VOLUME(vol_p);

  vol_p->id = id;
  error = initString(&vol_p->path, path_p->size);
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

  // the return volume
  *volume_p = vol_p;

end:

  if (error > 0) {
    error = NO_ERROR;
  }

  if (error != NO_ERROR && vol_p != NULL) {
    (void)destroyString(&vol_p->path);
    free(vol_p);
  }

  return error;
}

int formatVolume(const Volume *volume_p, VolumeType type)
{
  int error = NO_ERROR;
  offset_t ofst = 0;
  count_t page_count = 0;
  count_t page_map_page_count = 0;
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

  page_map_page_count = ALIGN_BYTES((ALIGN_BYTES(page_count, 8) >> 3), PAGE_SIZE) / PAGE_SIZE;

  header.page_map_start_page = 1; // 0 is the page to save header and ...

  header.page_map_end_page = page_map_page_count;
  header.current_alloc_page = page_map_page_count + 1;
  header.allocatd_page_count = page_map_page_count + 1;

  // header.file_tracker should be set after the btree is created.

  // move to the start of file
  ofst = seekFile(volume_p->fd, 0, SEEK_SET);
  if (ofst < 0) {
    error = (int)ofst;
    goto end;
  }

  // init the first page_map_page_count pages
  {
    const char page_initializer[PAGE_SIZE] = { 0 };

    for (int i = 0; i < page_map_page_count; ++i) {
      error = writeFile(volume_p->fd, (byte*)page_initializer, PAGE_SIZE);
      if (error != NO_ERROR) {
        goto end;
      }
    }
  }


  // TODO : b+tree for file_tracker

end:

  return error;
}

int allocPages(Volume *volume_p, id64_t *pages_p, count_t pages_count)
{
  int error = NO_ERROR;
  VolumeHeader header;
  count_t i = 0;
  id64_t current_page_idx = 0;

  assert(volume_p != NULL && pages_p != NULL && pages_count > 0);

  error = getVolumeHeader(volume_p, &header);
  if (error != NO_ERROR) {
    goto end;
  }

  if (header.allocatd_page_count + pages_count > header.page_count) {
    error = ER_VOLUME_NOT_ENOUGH_PAGE;
    goto end;
  }

  // can allocate continuous pages
  i = 0;
  current_page_idx = header.current_alloc_page;
  if (header.continuous_alloc) {
    for (; i < pages_count && current_page_idx < header.page_count; ++i) {
      pages_p[i] = current_page_idx++;
    }
  }

  // search non-continuous pages
  pages_count -= i;
  while (pages_count-- > 0) {
    ;
  }

end:

  return error;
}
