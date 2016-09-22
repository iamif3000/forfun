/*
 * base_string.c
 *
 *  Created on: Mar 11, 2016
 *      Author: duping
 */

#include <stdlib.h>
#include <string.h>

#include "../port/atomic.h"
#include "base_string.h"
#include "../common/common.h"

StringAlloc string_alloc = &malloc;
StringFree string_free = &free;

String *cloneString(String *str)
{
  String *ret_str = NULL;

  assert(str != NULL);

  ret_str = allocString(str->size, NULL);
  if (ret_str == NULL) {
    goto end;
  }

  (void)memcpy(ret_str->bytes, str->bytes, str->length);
  ret_str->bytes[str->length] = '\0';
  ret_str->length = str->length;
  ret_str->ref_count = 1;

end:

  return ret_str;
}

/*
 * size should be big enough to hold cstr and '\0'
 */
String *allocString(count_t size, const char *cstr)
{
  String *str = NULL;
  int i = 0;

  assert(size > 0);

  size = ALIGN_DEFAULT(size);

  str = (String*)string_alloc(sizeof(String));
  if (str == NULL) {
    goto end;
  }

  str->bytes = (byte*)string_alloc(size);
  if (str->bytes == NULL) {
    goto end;
  }

  if (cstr != NULL) {
    while (*cstr != '\0' && i < size - 1) {
      str->bytes[i] = *cstr;

      ++cstr;
      ++i;
    }
  }

  str->bytes[i] = '\0';

  str->size = size;
  str->length = i;
  str->ref_count = 1;
  str->lock = 0;

end:

  return str;
}

void freeString(String *str)
{
  if (str != NULL) {
    if (str->ref_count < 2) {
      if (str->bytes != NULL) {
        string_free(str->bytes);
      }
      string_free(str);
    } else {
      lock_int(&str->lock);

      --str->ref_count;
      if (str->ref_count < 1) {
        freeString(str);
      }

      unlock_int(&str->lock);
    }
  }
}

/*
 * for init String on stack
 * don't call refString on the String.
 * size should be big enough to hold cstr and '\0'
 */
int initString(String *str, count_t size, const char *cstr)
{
  int error = NO_ERROR;
  int i = 0;

  assert(str != NULL && size > 0);

  size = ALIGN_DEFAULT(size);

  str->bytes = (byte*)string_alloc(size);
  if (str->bytes == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    SET_ERROR(error);

    goto end;
  }

  if (cstr != NULL) {
    while (*cstr != '\0' && i < size - 1) {
      str->bytes[i] = *cstr;

      ++cstr;
      ++i;
    }
  }

  str->bytes[i] = '\0';

  str->length = i;
  str->ref_count = 1;
  str->size = size;
  str->lock = 0;

end:

  return error;
}

/*
 * for destroy String on stack
 * don't call refString on the String.
 */
int destroyString(String *str)
{
  int error = NO_ERROR;

  assert(str != NULL);

  if (str->bytes != NULL) {
    string_free(str->bytes);
    str->bytes = NULL;
  }

  return error;
}

/*
 * Call this to get a ref of String.
 * It's often used in multi-threads env.
 * Passing a ref to a new thread instead of passing the String
 */
String *refString(String *str)
{
  assert(str != NULL);

  lock_int(&str->lock);
  ++str->ref_count;
  unlock_int(&str->lock);

  return str;
}

/*
 * Call this function before editing the String to get the editable String,
 * unless you are sure this is the only ref before and after the editing.
 */
String *editString(String *str)
{
  String *ret_str = NULL;

  assert(str != NULL);

  if (str->ref_count > 1) {
    ret_str = cloneString(str);

    freeString(str);
  } else {
    ret_str = str;
  }

  return ret_str;
}

/*
 * NOTE: if this is not a String on stack, call editString before copy
 */
int copyString(String *dst, const String *src)
{
  int error = NO_ERROR;

  assert(dst != NULL && dst->bytes != NULL
        && src != NULL && src->bytes != NULL);

  if (dst->size < src->size) {
    (void)destroyString(dst);

    error = initString(dst, src->size, NULL);
    if (error != NO_ERROR) {
      goto end;
    }
  }

  memcpy(dst->bytes, src->bytes, src->length);
  dst->bytes[dst->length] = '\0';
  dst->length = src->length;

end:

  return error;
}

int appendByte(String *str, byte b)
{
  int error = NO_ERROR;
  byte *buf_p = NULL;
  count_t new_size = 0;

  assert(str != NULL);

  if (str->length + 1 >= str->size) {
    new_size = str->size * 1.5;
    buf_p = (byte*)string_alloc(sizeof(byte) * new_size);
    if (buf_p == NULL) {
      error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
      SET_ERROR(error);

      goto end;
    }

    memcpy(buf_p, str->bytes, str->length);
    buf_p[str->length] = '\0';

    string_free(str->bytes);

    str->bytes = buf_p;
    str->size = new_size;
  }

  str->bytes[str->length++] = b;
  str->bytes[str->length] = '\0';

end:

  return error;
}

int appendBytes(String *str, byte *b_p, count_t length)
{
  int error = NO_ERROR;
  byte *buf_p = NULL;
  count_t new_size = 0;

  assert(str != NULL && b_p != NULL && length > 0);

  if (str->length + length >= str->size) {
    new_size = str->size + length * 1.5;
    buf_p = (byte*)string_alloc(sizeof(byte) * new_size);
    if (buf_p == NULL) {
      error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
      SET_ERROR(error);

      goto end;
    }

    memcpy(buf_p, str->bytes, str->length);
    buf_p[str->length] = '\0';

    string_free(str->bytes);

    str->bytes = buf_p;
    str->size = new_size;
  }

  memcpy(str->bytes + str->length, b_p, length);
  str->length += length;

  str->bytes[str->length] = '\0';

end:

  return error;
}

void resetString(String *str)
{
  assert(str != NULL && str->bytes != NULL && str->size != 0 && str->ref_count == 1);

  str->length = 0;
  str->bytes[str->length] = '\0';
}

count_t stringLength(String *str)
{
  assert(str != NULL);

  return str->length;
}

byte *stringToStream(byte *buf_p, String *str)
{
  count_t len = 0;

  assert(buf_p != NULL && str != NULL);

  len = htonll(str->length);
  memcpy(buf_p, &len, sizeof(len));
  buf_p += sizeof(len);

  memcpy(buf_p, str->bytes, str->length);
  buf_p += str->length;

  return buf_p;
}

// The String must be a stack value.
byte *streamToString(byte *buf_p, String *str, int *error)
{
  count_t len = 0;
  byte *buf2_p = NULL;

  assert(buf_p != NULL && str != NULL && error != NULL);

  memcpy(&len, buf_p, sizeof(len));
  buf_p += sizeof(len);
  len = ntohll(len);

  if (str->size <= len) {
    buf2_p = (byte*)string_alloc(len + 1);
    if (buf2_p == NULL) {
      *error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
      SET_ERROR(*error);
      goto end;
    }

    string_free(str->bytes);
    str->bytes = buf2_p;
  }

  memcpy(str->bytes, buf_p, len);
  buf_p += len;
  str->bytes[len] = '\0';
  str->length = len;
  str->size = len + 1;

end:

  return buf_p;
}
