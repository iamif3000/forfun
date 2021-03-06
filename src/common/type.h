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
typedef __uint64_t uid64_t;
typedef __uint64_t idx64_t;

typedef __int64_t int64_t;
typedef __int64_t id64_t;
typedef __int64_t offset_t;

typedef int bool;
typedef union value Value;

#if __WORDSIZE == 64
  typedef uint64_ptr uint_ptr;
#else
  typedef uint32_ptr uint_ptr;
#endif

union value {
  char c;
  byte uc;
  short s;
  uint16_t ui16;
  int i;
  uint32_t ui;
  int64_t i64;
  uint64_t ui64;
  float f;
  double d;
  void *ptr;
};

#endif
