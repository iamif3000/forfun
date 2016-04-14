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
  byte *p = (byte*)&ui64;
  SWAP(p[0], p[7]);
  SWAP(p[1], p[6]);
  SWAP(p[2], p[5]);
  SWAP(p[3], p[4]);

  return ui64;
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
  byte *p = (byte*)&ui32;
  SWAP(p[0], p[3]);
  SWAP(p[1], p[2]);

  return ui32;
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
  byte *p = (byte*)&ui16;
  SWAP(p[0], p[1]);

  return ui16;
}

uint16_t htons(uint16_t ui16)
{
  return ntohl(ui16);
}
