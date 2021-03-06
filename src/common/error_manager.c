/*
 * error_manager.c
 *
 *  Created on: Apr 15, 2016
 *      Author: duping
 */

#include<stdlib.h>
#include <string.h>

#include "../common/common.h"
#include "../common/error_manager.h"
#include "../common/block_alloc_helper.h"

#include "../port/atomic.h"
#include "../port/thread.h"

#include "error_manager.h"

#define ERROR_LIST_PER_BLOCK 256

typedef struct error_list_block ErrorListBlock;

struct error_list_block {
  ErrorListBlock *next_p;
  ErrorList list_array[ERROR_LIST_PER_BLOCK];
};

static volatile int error_list_clock_lock = 0;
static ErrorListBlock *error_list_clock_p = NULL;
static ErrorList* free_error_list_p = NULL;

static ErrorList *allocErrorList();
static void initErrorList(ErrorList *list_p);
static void freeErrorList(ErrorList *list_p);
static int allocErrorListBlock();

/*
 * static
 */
ErrorList *allocErrorList()
{
  int error = NO_ERROR;
  ErrorList *list_p = NULL;

  while (true) {
    BAH_ALLOC_FROM_FREE_LIST(next_p, free_error_list_p, list_p);

    if (list_p == NULL) {
      BAH_LOCK_AND_ALLOC_BLOCK(error_list_clock_lock, free_error_list_p, allocErrorListBlock, error);

      if (error != NO_ERROR) {
        break;
      }
    } else {
      initErrorList(list_p);
      break;
    }
  }

  return list_p;
}

/*
 * static
 */
void initErrorList(ErrorList *list_p)
{
  assert(list_p != NULL);

  list_p->error_code = NO_ERROR;
  list_p->line = 0;
  list_p->next_p = NULL;
  list_p->file_name[0] = '\0';
}

/*
 * static
 */
void freeErrorList(ErrorList *list_p)
{
  ErrorList *next_p = NULL;

  assert(list_p != NULL);

  BAH_FREE_TO_FREE_LIST(next_p, free_error_list_p, list_p, next_p);
}

/*
 * static
 */
int allocErrorListBlock()
{
  int error = NO_ERROR;
  ErrorListBlock *block_p = NULL;
  ErrorList *list_p = NULL;

  block_p = (ErrorListBlock*)malloc(sizeof(ErrorListBlock));
  if (block_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  BAH_LINK_BLOCK(next_p, error_list_clock_p, block_p, list_array, ERROR_LIST_PER_BLOCK, next_p, list_p);
  BAH_LINK_FREE_LIST(next_p, free_error_list_p, &block_p->list_array[0], &block_p->list_array[ERROR_LIST_PER_BLOCK - 1], list_p);

end:

  return error;
}

int initErrorManagerEnv()
{
  int error = NO_ERROR;

  error = allocErrorListBlock();

  return error;
}

void destroyErrorManagerEnv()
{
  ErrorListBlock *next_p = NULL;

  while (error_list_clock_p != NULL) {
    next_p = error_list_clock_p->next_p;

    free(error_list_clock_p);

    error_list_clock_p = next_p;
  }
}

int recordError(const int error_code, const char *file_p, const int line)
{
  int error = NO_ERROR;
  ErrorList *list_p = NULL;
  int len;
  Thread *thread_p = NULL;

  assert(error_code < 0 && file_p != NULL && line > 0);

  thread_p = getThread();
  if (thread_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  list_p = allocErrorList();
  if (list_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  len = strlen(file_p);

  list_p->error_code = error_code;
  list_p->line = line;

  if (len < FILE_NAME_LEN) {
    memcpy(&list_p->file_name[0], file_p, FILE_NAME_LEN);
  } else {
    memcpy(&list_p->file_name[0], file_p + len - FILE_NAME_LEN, FILE_NAME_LEN);
  }

  // current thread's error list
  list_p->next_p = thread_p->err_list_p;
  thread_p->err_list_p = list_p;

end:

  return error;
}

int lastError()
{
  int error = NO_ERROR;
  Thread *thread_p = NULL;
  ErrorList *list_p = NULL;

  thread_p = getThread();
  if (thread_p == NULL) {
    // this should be serious error, so when meet this error we should exit...
    // So this doesn't matter
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  list_p = thread_p->err_list_p;
  if (list_p != NULL) {
    error = list_p->error_code;
  }

end:

  return error;
}

int removeLastError()
{
  int error = NO_ERROR;
  Thread *thread_p = NULL;
  ErrorList *list_p = NULL;

  thread_p = getThread();
  if (thread_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  list_p = thread_p->err_list_p;
  if (list_p != NULL) {
    thread_p->err_list_p = list_p->next_p;

    freeErrorList(list_p);
  }

end:

  return error;
}

int clearErrors()
{
  int error = NO_ERROR;
  Thread *thread_p = NULL;
  ErrorList *next_p = NULL;

  thread_p = getThread();
  if (thread_p == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  while (thread_p->err_list_p != NULL) {
    next_p = thread_p->err_list_p->next_p;

    freeErrorList(thread_p->err_list_p);

    thread_p->err_list_p = next_p;
  }

end:

  return error;
}

bool hasError()
{
  Thread *thread_p = NULL;
  ErrorList *next_p = NULL;

  thread_p = getThread();
  if (thread_p == NULL) {
    return true;
  }

  if (thread_p->err_list_p != NULL) {
    return true;
  }

  return false;
}

int getErrorListBlockCount()
{
  int i = 0;
  ErrorListBlock *block_p = error_list_clock_p;

  while (block_p != NULL) {
    ++i;

    block_p = block_p->next_p;
  }

  return i;
}
