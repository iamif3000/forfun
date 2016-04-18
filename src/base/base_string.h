/*
 * base_string.h - NULL terminated string with length
 *
 *  Created on: Mar 11, 2016
 *      Author: duping
 */

#include "../common/type.h"

#ifndef BASE_STRING_H_
#define BASE_STRING_H_

#define INIT_STRING(str_p) \
  do { \
    (str_p)->length = 0; \
    (str_p)->size = 0; \
    (str_p)->ref_count = 0; \
    (str_p)->bytes = NULL; \
    (str_p)->lock = 0; \
  } while (0)

#define freeStringAndSetNull(str) \
  do { \
    freeString(str); \
    (str) = NULL; \
  } while(0)

typedef void*(*StringAlloc)(count_t);
typedef void(*StringFree)(void*);
typedef struct base_string String;

struct base_string {
  count_t length;
  count_t size;
  count_t ref_count;
  byte *bytes;
  volatile int lock;
};

// var
extern String empty_string;
extern StringAlloc string_alloc;
extern StringFree string_free;

// func
String *allocString(count_t size, const char *cstr);
void freeString(String *str);
String *refString(String *str);
String *editString(String *str);
String *cloneString(String *str);
int copyString(String *dst, const String *src);
int appendByte(String *str, byte b);
void resetString(String *str);

int initString(String *str, count_t size, const char *str);
int destroyString(String *str);

#endif /* BASE_STRING_H_ */
