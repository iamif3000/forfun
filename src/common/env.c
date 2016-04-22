/*
 * env.c
 *
 *  Created on: Apr 22, 2016
 *      Author: duping
 */

#include "common.h"
#include "env.h"

static char *workDir = NULL;
static char *confDir = NULL;
static char *dataDir = NULL;

int initEnv(char *_workDir, char *_confDir, char *_dataDir)
{
  int error = NO_ERROR;

  // TODO

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

char *getWorkingDir()
{
  return workDir;
}

char *getConfDir()
{
  return confDir;
}

char *getDataDir()
{
  return dataDir;
}
