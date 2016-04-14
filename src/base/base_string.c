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
  ret_str->length = str->length;
  ret_str->ref_count = 1;

end:

  return ret_str;
}

String *allocString(count_t size, const char *cstr)
{
  String *str = NULL;

  assert(size > 0);

  size = ALIGN_DEFAULT(size + 1);

  str = (String*)string_alloc(sizeof(String));
  if (str == NULL) {
    goto end;
  }

  str->bytes = (byte*)string_alloc(size);
  if (str->bytes == NULL) {
    goto end;
  }

  if (cstr != NULL) {
    int i = 0;

    while (*cstr != '\0' && i < size) {
      str->bytes[i] = *cstr;

      ++cstr;
      ++i;
    }

    str->bytes[i] = '\0';
  }

  str->size = size;
  str->length = 0;
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
 */
int initString(String *str, count_t size)
{
  int error = NO_ERROR;

  assert(str != NULL && size > 0);

  size = ALIGN_DEFAULT(size);

  str->bytes = (byte*)string_alloc(size);
  if (str->bytes == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  str->length = 0;
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

    error = initString(dst, src->size);
    if (error != NO_ERROR) {
      goto end;
    }
  }

  memcpy(dst->bytes, src->bytes, src->length);
  dst->length = src->length;

end:

  return error;
}