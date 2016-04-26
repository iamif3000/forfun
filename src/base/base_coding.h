/*
 * base_coding.h
 *
 *  Created on: Apr 25, 2016
 *      Author: duping
 */

#include "../common/type.h"

#ifndef BASE_CODING_H_
#define BASE_CODING_H_

typedef void*(*BaseAlloc)(count_t);
typedef void(*BaseFree)(void*);

extern BaseAlloc base_alloc;
extern BaseFree base_free;

byte *base64_encode(byte *src_p, count_t length);
byte *base64_decode(byte *src_p, count_t length);

byte *base32_encode(byte *src_p, count_t length);
byte *base32_decode(byte *src_p, count_t length);

byte *base16_encode(byte *src_p, count_t length);
byte *base16_decode(byte *src_p, count_t length);


#endif /* BASE_CODING_H_ */
