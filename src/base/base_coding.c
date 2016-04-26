/*
 * base_coding.c
 *
 *  Created on: Apr 25, 2016
 *      Author: duping
 *
 *  reference : https://tools.ietf.org/html/rfc4648
 */

#include <stdlib.h>
#include <string.h>

#include "../common/common.h"

#include "base_coding.h"

BaseAlloc base_alloc = &malloc;
BaseFree base_free = &free;

static byte base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "abcdefghijklmnopqrstuvwxyz"
                             "0123456789+/";
static byte base64_padding = '=';

static byte base32_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "234567";
static byte base32_padding = '=';

static byte base16_table[] = "0123456789"
                             "ABCDEF";

static inline char getBase64Base(byte b);
static inline char getBase32Base(byte b);
static inline char getBase16Base(byte b);

/*
 * static
 */
char getBase64Base(byte b) {
  char c = -1; // invalid value

  if (b >= 'A' && b <= 'Z') {
    c = b - 'A';
  } else if (b >= 'a' && b <= 'z') {
    c = b - 'a' + 26;
  } else if (b >= '0' && b <= '9') {
    c = b - '0' + 52;
  } else if (b == '+') {
    c = 62;
  } else if (b == '/') {
    c = 63;
  }

  return c;
}

/*
 * static
 */
char getBase32Base(byte b)
{
  char c = -1;

  if (b >= 'A' && b <= 'Z') {
    c = b - 'A';
  } else if (b >= '2' && b <= '7') {
    c = b - '2' + 26;
  }

  return c;
}

/*
 * static
 */
char getBase16Base(byte b)
{
  char c = -1;

  if (b >= '0' && b <= '9') {
    c = b - '0';
  } else if (b >= 'A' && b <= 'F') {
    c = b - 'A' + 10;
  }

  return c;
}

byte *base64_encode(byte *src_p, count_t length)
{
  byte *encoded_p = NULL;
  count_t encode_len = 0;
  count_t i, j, remain_len;

  if (src_p == NULL) {
    goto end;
  } else {
    encode_len = ((length + 2)/3) << 2;
    encoded_p = (byte*)base_alloc(encode_len + 1);
    if (encoded_p == NULL) {
      SET_ERROR(ER_GENERIC_OUT_OF_VIRTUAL_MEMORY);
      goto end;
    }

    remain_len = length % 3;
    length -= remain_len;

    i = j = 0;
    while (i < length) {
      encoded_p[j++] = base64_table[src_p[i] >> 2];
      encoded_p[j++] = base64_table[((src_p[i] & 0x03) << 4) | (src_p[i + 1] >> 4)];
      encoded_p[j++] = base64_table[((src_p[i + 1] & 0x0F) << 2) | src_p[i + 2] >> 6];
      encoded_p[j++] = base64_table[src_p[i + 2] & 0x3F];

      i += 3;
    }

    if (remain_len == 1) {
      encoded_p[j++] = base64_table[src_p[i] >> 2];
      encoded_p[j++] = base64_table[(src_p[i] & 0x03) << 4];
      encoded_p[j++] = base64_padding;
      encoded_p[j] = base64_padding;
    } else if (remain_len == 2) {
      encoded_p[j++] = base64_table[src_p[i] >> 2];
      encoded_p[j++] = base64_table[((src_p[i] & 0x03) << 4) | (src_p[i + 1] >> 4)];
      encoded_p[j++] = base64_table[(src_p[i + 1] & 0x0F) << 2];
      encoded_p[j] = base64_padding;
    }

    encoded_p[encode_len] = '\0';
  }

end:

  return encoded_p;
}

byte *base64_decode(byte *src_p, count_t length)
{
#define GET_BASE_CODE(b) \
  do { \
    base_code = getBase64Base(b); \
    if (base_code < 0) { \
      error = ER_BASE_BASE64_INVALID_BYTE; \
      SET_ERROR(error); \
      goto end; \
    } \
  } while(0)

  int error = NO_ERROR;
  byte *decoded_p = NULL;
  char base_code;
  count_t decode_len = 0;
  count_t i, j;

  if (src_p == NULL) {
    goto end;
  } else {
    if ((length & ~0x03) != length) {
      error = ER_BASE_BASE_INVALID_LEN;
      SET_ERROR(error);
      goto end;
    }

    decode_len = (length >> 2) * 3;
    decoded_p = (byte*)base_alloc(decode_len + 1);
    if (decoded_p == NULL) {
      error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
      SET_ERROR(error);
      goto end;
    }

    i = j = 0;
    while (i + 4 < length) {
      GET_BASE_CODE(src_p[i++]);

      decoded_p[j] = base_code << 2;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j++] |= base_code >> 4;
      decoded_p[j] = base_code << 4;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j++] |= base_code >> 2;
      decoded_p[j] = base_code << 6;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j++] |= base_code & 0x3F;
    }

    // last 4 bytes
    if (i < length) {
      GET_BASE_CODE(src_p[i++]);

      decoded_p[j] = base_code << 2;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j++] |= base_code >> 4;
      decoded_p[j] = base_code << 4;

      if (src_p[i] != base64_padding) {
        GET_BASE_CODE(src_p[i++]);

        decoded_p[j++] |= base_code >> 2;
        decoded_p[j] = base_code << 6;

        if (src_p[i] != base64_padding) {
          GET_BASE_CODE(src_p[i++]);

          decoded_p[j] |= base_code & 0x3F;
        }
      }

      while (i < length) {
        if (src_p[i] != base64_padding) {
          error = ER_BASE_BASE64_INVALID_BYTE;
          SET_ERROR(error);
          goto end;
        }

        ++i;
      }

      decoded_p[j + 1] = '\0';
    }

    decoded_p[decode_len] = '\0';
  }

end:

  if (error != NO_ERROR && decoded_p != NULL) {
    base_free(decoded_p);
    decoded_p = NULL;
  }

  return decoded_p;

#undef GET_BASE_CODE
}

byte *base32_encode(byte *src_p, count_t length)
{
  byte *encoded_p = NULL;
  count_t encode_len = 0;
  count_t i, j, remain_len;

  if (src_p == NULL) {
    goto end;
  } else {
    encode_len = ((length + 4)/5) << 3;
    encoded_p = (byte*)base_alloc(encode_len + 1);
    if (encoded_p == NULL) {
      SET_ERROR(ER_GENERIC_OUT_OF_VIRTUAL_MEMORY);
      goto end;
    }

    remain_len = length % 5;
    length -= remain_len;

    i = j = 0;
    while (i < length) {
      encoded_p[j++] = base32_table[src_p[i] >> 3];
      encoded_p[j++] = base32_table[((src_p[i] & 0x07) << 2) | (src_p[i + 1] >> 6)];
      encoded_p[j++] = base32_table[(src_p[i + 1] & 0x03F) >> 1];
      encoded_p[j++] = base32_table[((src_p[i + 1] & 0x01) << 4) | (src_p[i + 2] >> 4)];
      encoded_p[j++] = base32_table[((src_p[i + 2] & 0x0F) << 1) | (src_p[i + 3] >> 7)];
      encoded_p[j++] = base32_table[(src_p[i + 3] & 0x7F) >> 2];
      encoded_p[j++] = base32_table[((src_p[i + 3] & 0x03) << 3) | (src_p[i + 4] >> 5)];
      encoded_p[j++] = base32_table[src_p[i + 4] & 0x1F];

      i += 5;
    }

    if (remain_len == 1) {
      encoded_p[j++] = base32_table[src_p[i] >> 3];
      encoded_p[j++] = base32_table[(src_p[i] & 0x07) << 2];
    } else if (remain_len == 2) {
      encoded_p[j++] = base32_table[src_p[i] >> 3];
      encoded_p[j++] = base32_table[((src_p[i] & 0x07) << 2) | (src_p[i + 1] >> 6)];
      encoded_p[j++] = base32_table[(src_p[i + 1] & 0x3F) >> 1];
      encoded_p[j++] = base32_table[(src_p[i + 1] & 0x01) << 4];
    } else if (remain_len == 3) {
      encoded_p[j++] = base32_table[src_p[i] >> 3];
      encoded_p[j++] = base32_table[((src_p[i] & 0x07) << 2) | (src_p[i + 1] >> 6)];
      encoded_p[j++] = base32_table[(src_p[i + 1] & 0x3F) >> 1];
      encoded_p[j++] = base32_table[((src_p[i + 1] & 0x01) << 4) | (src_p[i + 2] >> 4)];
      encoded_p[j++] = base32_table[((src_p[i + 2] & 0x0F) << 1)];
    } else if (remain_len == 4) {
      encoded_p[j++] = base32_table[src_p[i] >> 3];
      encoded_p[j++] = base32_table[((src_p[i] & 0x07) << 2) | (src_p[i + 1] >> 6)];
      encoded_p[j++] = base32_table[(src_p[i + 1] & 0x3F) >> 1];
      encoded_p[j++] = base32_table[((src_p[i + 1] & 0x01) << 4) | (src_p[i + 2] >> 4)];
      encoded_p[j++] = base32_table[((src_p[i + 2] & 0x0F) << 1) | (src_p[i + 3] >> 7)];
      encoded_p[j++] = base32_table[(src_p[i + 3] & 0x7F) >> 2];
      encoded_p[j++] = base32_table[(src_p[i + 3] & 0x03) << 3];
    }

    if ((j & 0x7) != 0) {
      for (i = (j & 0x7); i < 8; ++i) {
        encoded_p[j++] = base32_padding;
      }
    }

    encoded_p[encode_len] = '\0';
  }

end:

  return encoded_p;
}

byte *base32_decode(byte *src_p, count_t length)
{
#define GET_BASE_CODE(b) \
  do { \
    base_code = getBase32Base(b); \
    if (base_code < 0) { \
      error = ER_BASE_BASE32_INVALID_BYTE; \
      SET_ERROR(error); \
      goto end; \
    } \
  } while(0)

  int error = NO_ERROR;
  byte *decoded_p = NULL;
  char base_code;
  count_t decode_len = 0;
  count_t i, j;

  if (src_p == NULL) {
    goto end;
  } else {
    if ((length & ~0x07) != length) {
      error = ER_BASE_BASE_INVALID_LEN;
      SET_ERROR(error);
      goto end;
    }

    decode_len = (length >> 3) * 5;
    decoded_p = (byte*)base_alloc(decode_len + 1);
    if (decoded_p == NULL) {
      error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
      SET_ERROR(error);
      goto end;
    }

    i = j = 0;
    while (i + 8 < length) {
      GET_BASE_CODE(src_p[i++]);

      decoded_p[j] = base_code << 3;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j++] |= base_code >> 2;
      decoded_p[j] = base_code << 6;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j] |= base_code << 1;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j++] |= base_code >> 4;
      decoded_p[j] = base_code << 4;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j++] |= base_code >> 1;
      decoded_p[j] = base_code << 7;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j] |= base_code << 2;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j++] |= base_code >> 3;
      decoded_p[j] = base_code << 5;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j++] |= base_code;
    }

    // last 8 bytes
    if (i < length) {
      GET_BASE_CODE(src_p[i++]);

      decoded_p[j] = base_code << 3;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j++] |= base_code >> 2;
      decoded_p[j] = base_code << 6;

      if (src_p[i] != base32_padding) {
        GET_BASE_CODE(src_p[i++]);

        decoded_p[j] |= base_code << 1;

        GET_BASE_CODE(src_p[i++]);

        decoded_p[j++] |= base_code >> 4;
        decoded_p[j] = base_code << 4;

        if (src_p[i] != base32_padding) {
          GET_BASE_CODE(src_p[i++]);

          decoded_p[j++] |= base_code >> 1;
          decoded_p[j] = base_code << 7;

          if (src_p[i] != base32_padding) {
            GET_BASE_CODE(src_p[i++]);

            decoded_p[j] |= base_code << 2;

            GET_BASE_CODE(src_p[i++]);

            decoded_p[j++] |= base_code >> 3;
            decoded_p[j] = base_code << 5;

            if (src_p[i] != base32_padding) {
              GET_BASE_CODE(src_p[i++]);

              decoded_p[j] |= base_code;
            }
          }
        }
      }

      while (i < length) {
        if (src_p[i] != base32_padding) {
          error = ER_BASE_BASE32_INVALID_BYTE;
          SET_ERROR(error);
          goto end;
        }

        ++i;
      }

      decoded_p[j + 1] = '\0';
    }

    decoded_p[decode_len] = '\0';
  }

end:

  if (error != NO_ERROR && decoded_p != NULL) {
    base_free(decoded_p);
    decoded_p = NULL;
  }

  return decoded_p;

#undef GET_BASE_CODE
}

byte *base16_encode(byte *src_p, count_t length)
{
  byte *encoded_p = NULL;
  count_t encode_len = 0;
  count_t i, j;

  if (src_p == NULL) {
    goto end;
  } else {
    encode_len = length << 1;
    encoded_p = (byte*)base_alloc(encode_len + 1);
    if (encoded_p == NULL) {
      SET_ERROR(ER_GENERIC_OUT_OF_VIRTUAL_MEMORY);
      goto end;
    }

    i = j = 0;
    while (i < length) {
      encoded_p[j++] = base16_table[src_p[i] >> 4];
      encoded_p[j++] = base16_table[src_p[i] & 0x0F];

      ++i;
    }

    encoded_p[encode_len] = '\0';
  }

end:

  return encoded_p;
}

byte *base16_decode(byte *src_p, count_t length)
{
#define GET_BASE_CODE(b) \
  do { \
    base_code = getBase16Base(b); \
    if (base_code < 0) { \
      error = ER_BASE_BASE16_INVALID_BYTE; \
      SET_ERROR(error); \
      goto end; \
    } \
  } while(0)

  int error = NO_ERROR;
  byte *decoded_p = NULL;
  char base_code;
  count_t decode_len = 0;
  count_t i, j;

  if (src_p == NULL) {
    goto end;
  } else {
    if ((length & ~0x01) != length) {
      error = ER_BASE_BASE_INVALID_LEN;
      SET_ERROR(error);
      goto end;
    }

    decode_len = length >> 1;
    decoded_p = (byte*)base_alloc(decode_len + 1);
    if (decoded_p == NULL) {
      error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
      SET_ERROR(error);
      goto end;
    }

    i = j = 0;
    while (i < length) {
      GET_BASE_CODE(src_p[i++]);

      decoded_p[j] = base_code << 4;

      GET_BASE_CODE(src_p[i++]);

      decoded_p[j++] |= base_code;
    }

    decoded_p[decode_len] = '\0';
  }

end:

  if (error != NO_ERROR && decoded_p != NULL) {
    base_free(decoded_p);
    decoded_p = NULL;
  }

  return decoded_p;

#undef GET_BASE_CODE
}
