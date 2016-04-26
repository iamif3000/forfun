/*
 * env.c
 *
 *  Created on: Apr 22, 2016
 *      Author: duping
 */

#include <string.h>

#include "common.h"
#include "env.h"

static char *workDir = NULL;
static char *confDir = NULL;
static char *dataDir = NULL;

int initEnv(char *_workDir, char *_confDir, char *_dataDir)
{
  int error = NO_ERROR;

  assert(_workDir != NULL && _confDir != NULL && _dataDir != NULL);

  // TODO : more
  workDir = strdup(_workDir);
  if (workDir == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  confDir = strdup(_confDir);
  if (workDir == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

  dataDir = strdup(_dataDir);
  if (workDir == NULL) {
    error = ER_GENERIC_OUT_OF_VIRTUAL_MEMORY;
    goto end;
  }

end:

  return error;
}

void destroyEnv()
{
  if (workDir != NULL) {
    freeAndSetNull(workDir);
  }

  if (confDir != NULL) {
    freeAndSetNull(confDir);
  }

  if (dataDir != NULL) {
    freeAndSetNull(dataDir);
  }
}

const char *getWorkingDir()
{
  return workDir;
}

const char *getConfDir()
{
  return confDir;
}

const char *getDataDir()
{
  return dataDir;
}
