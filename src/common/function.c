/*
 * function.c
 *
 *  Created on: Mar 14, 2016
 *      Author: duping
 */

#include "function.h"

bool is_bigendian()
{
  static bool is_be = -1; // invalid value

  if (is_be != -1) {
    return is_be;
  }

  // check
  uint16_t us = 0x0001;
  byte *p = (byte*)&us;

  if (p[0] > 0) {
    is_be = false;
  } else {
    is_be = true;
  }

  return is_be;
}

uint64_t ntohll(uint64_t ui64)
{
  if (is_bigendian()) {
    return ui64;
  }

  // 8 bytes
  return ((ui64 << 56) & 0xFF00000000000000LL) |
         ((ui64 << 40) & 0x00FF000000000000LL) |
         ((ui64 << 24) & 0x0000FF0000000000LL) |
         ((ui64 << 8)  & 0x000000FF00000000LL) |
         ((ui64 >> 8)  & 0x00000000FF000000LL) |
         ((ui64 >> 24) & 0x0000000000FF0000LL) |
         ((ui64 >> 40) & 0x000000000000FF00LL) |
         ((ui64 >> 56) & 0x00000000000000FFLL);
}

uint64_t htonll(uint64_t ui64)
{
  return ntohll(ui64);
}

uint32_t ntohl(uint32_t ui32)
{
  if (is_bigendian()) {
    return ui32;
  }

  // 4 bytes
  return ((ui32 << 24) & 0xFF000000) |
         ((ui32 << 8)  & 0x00FF0000) |
         ((ui32 >> 8)  & 0x0000FF00) |
         ((ui32 >> 24) & 0x000000FF);
}

uint32_t htonl(uint32_t ui32)
{
  return ntohl(ui32);
}

uint16_t ntohs(uint16_t ui16)
{
  if (is_bigendian()) {
    return ui16;
  }

  // 2 bytes
  return ((ui16 << 8) & 0xFF00) |
         ((ui16 >> 8) & 0x00FF);
}

uint16_t htons(uint16_t ui16)
{
  return ntohl(ui16);
}
