/*
 * error_manager.c
 *
 *  Created on: Apr 15, 2016
 *      Author: duping
 */

#include "../common/common.h"

#include "../port/atomic.h"
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
    while (true) {
      list_p = free_error_list_p;
      if (list_p == NULL) {
        break;
      }

      if (MARKED(list_p) || !compare_and_swap_bool(&free_error_list_p, list_p, MARK(list_p))) {
        continue;
      }

      if (compare_and_swap_bool(&free_error_list_p, MARKED(list_p), list_p->next_p)) {
        break;
      }

      assert(false);
    }

    if (list_p == NULL) {
      lock_int(&error_list_clock_lock);

      if (free_error_list_p == NULL) {
        error = allocErrorListBlock();
      }

      unlock_int(&error_list_clock_lock);

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

  while (true) {
    next_p = free_error_list_p;
    if (MARKED(next_p)) {
      continue;
    }

    list_p->next_p = next_p;
    if (compare_and_swap_bool(&free_error_list_p, next_p, list_p)) {
      break;
    }
  }
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

  list_p = &block_p->list_array[0];
  for (int i = 1; i < ERROR_LIST_PER_BLOCK; ++i) {
    list_p[i-1].next_p = &list_p[i];
  }

  list_p[ERROR_LIST_PER_BLOCK - 1].next_p = NULL;

  block_p->next_p = error_list_clock_p;
  error_list_clock_p = block_p;

  // link free list
  while (true) {
    list_p = free_error_list_p;
    if (MARKED(list_p)) {
      continue;
    }

    block_p->list_array[ERROR_LIST_PER_BLOCK - 1].next_p = list_p;
    if (compare_and_swap_bool(&free_error_list_p, list_p, &block_p->list_array[0])) {
      break;
    }
  }

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

ErrorManager *createErrorManager()
{
  ErrorManager *manager_p = NULL;

  manager_p = (ErrorManager*)malloc(sizeof(ErrorManager));
  if (manager_p != NULL) {
    manager_p->head_p = NULL;
  }

  return manager_p;
}

void destroyErrorManager(ErrorManager *manager_p)
{
  assert(manager_p != NULL);

  free(manager_p);
}

int recordError(const int error_code, const char *file_p, const int line)
{
  int error = NO_ERROR;
  ErrorList *list_p = NULL;
  int len;

  assert(error_code < 0 && file_p != NULL && line > 0);

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

  // TODO : current thread's error list
  //list_p->next_p =

end:

  return error;
}
