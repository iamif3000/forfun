#ifndef ERROR_H_
#define ERROR_H_

enum {
  NO_ERROR = 0,

  ER_GENERIC = -9999,
  ER_GENERIC_OUT_OF_VIRTUAL_MEMORY,

  ER_SLOT_PAGE_NOT_ENOUGH_SPACE = -9980,
  ER_SLOT_PAGE_SLOT_OUT_OF_RANGE,

  ER_OS_FILE_CREATE_FAIL = -9960,
  ER_OS_FILE_SEEK_FAIL,
  ER_OS_FILE_REMOVE_FAIL,
  ER_OS_FILE_OPEN_FAIL,
  ER_OS_FILE_CLOSE_FAIL,
  ER_OS_FILE_READ_FAIL,
  ER_OS_FILE_WRITE_FAIL,

  ER_VOLUME_CREATE_VOLUME_FAIL = -9940,
  ER_VOLUME_NOT_ENOUGH_PAGE,

  ER_HASH_NO_SUCH_KEY = -9920,
  ER_HASH_DUPLICATE_KEY,
  ER_HASH_ITEM_LOCK_FAIL,

  ER_RQ_CREATE_FAIL = -9900,
  ER_RQ_LOCK_ERROR,
  ER_RQ_LOCK_TIMEOUT,
  ER_RQ_ABNORMAL_QUIT,

  ER_PC_NO_MORE_CACHE_ROOM = -9980,

  ER_VM_CONF_ERROR = -9960,
  ER_VM_NO_VOLUMETYPE_VOLUME,

  ER_BASE_BASE_INVALID_LEN = -9940,
  ER_BASE_BASE64_INVALID_BYTE,
  ER_BASE_BASE32_INVALID_BYTE,
  ER_BASE_BASE16_INVALID_BYTE,

  ER_LAST
};

#endif
