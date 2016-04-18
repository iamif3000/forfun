#include <ctype.h>

#ifndef TYPE_H_
#define TYPE_H_

#define TRUE 1
#define true 1
#define FALSE 0
#define false 0

typedef unsigned char byte;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef uint32_t uint32_ptr;
typedef __uint64_t uint64_t;
typedef __uint64_t number_t;
typedef __uint64_t count_t;
typedef __uint64_t uint64_ptr;

typedef __int64_t int64_t;
typedef __int64_t id64_t;
typedef __int64_t offset_t;

typedef int bool;

#if __WORDSIZE == 64
  typedef uint64_ptr uint_ptr;
#else
  typedef uint32_ptr uint_ptr;
#endif

#endif
