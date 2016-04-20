/*
 * error_manager.h
 *
 *  Created on: Apr 15, 2016
 *      Author: duping
 */

#include "error.h"

#ifndef ERROR_MANAGER_H_
#define ERROR_MANAGER_H_

#define FILE_NAME_LEN 64

#define destroyErrorManagerAndSetNull(manager_p) \
  do { \
    destroyErrorManager(manager_p); \
    manager_p = NULL; \
  } while(0)

#define SET_ERROR(error_code) recordError(error_code, __FILE__, __LINE__)

typedef struct error_list ErrorList;
typedef struct error_manager ErrorManager;

struct error_list {
  int error_code;
  int line;
  char file_name[FILE_NAME_LEN];
  ErrorList *next_p;
};

// per thread
struct error_manager {
  ErrorList *head_p;
};

int initErrorManagerEnv();
void destroyErrorManagerEnv();

ErrorManager *createErrorManager();
void destroyErrorManager(ErrorManager *manager_p);

int recordError(const int error_code, const char *file_p, const int line);

#endif /* ERROR_MANAGER_H_ */
