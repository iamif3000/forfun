/*
 * function.h
 *
 *  Created on: Mar 10, 2016
 *      Author: duping
 */

#include "type.h"

#ifndef FUNCTION_H_
#define FUNCTION_H_

#define freeAndSetNull(p) \
  do { \
    free(p); \
    p = NULL; \
  } while(0)

#define SWAP(a, b) \
  do { \
    (a) = (a)^(b); \
    (b) = (a)^(b); \
    (a) = (a)^(b); \
  } while(0)

#define OFFSET_OF(type, member) ((offset_t)(&((type*)0)->member))
#define GET_TYPE_PTR_BY_MEMBER_PTR(type, member, member_pointer) \
  ((type*)((byte*)(member_pointer) - OFFSET_OF(type, member)))

inline bool is_bigendian();
inline uint64_t ntohll(uint64_t ui64);
inline uint64_t htonll(uint64_t ui64);
inline uint32_t ntohl(uint32_t ui32);
inline uint32_t htonl(uint32_t ui32);
inline uint16_t ntohs(uint16_t ui16);
inline uint16_t htons(uint16_t ui16);

#endif /* FUNCTION_H_ */
