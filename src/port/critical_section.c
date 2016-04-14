/*
 * critical_section.c
 *
 *  Created on: Apr 1, 2016
 *      Author: duping
 */

#include "../common/common.h"

#include "critical_section.h"

CriticalSection *createCriticalSection(const char *name_p)
{
  CriticalSection *cs_p = NULL;

  cs_p = malloc(sizeof(CriticalSection));
  if (cs_p != NULL) {
    cs_p->rq_p = createResourceQueue(cs_p, name_p);
    if (cs_p->rq_p == NULL) {
      destroyCriticalSectionAndSetNull(cs_p);
    }
  }

  return cs_p;
}

void destroyCriticalSection(CriticalSection *cs_p)
{
  if (cs_p != NULL) {
    if (cs_p->rq_p != NULL) {
      destroyResourceQueueAndSetNull(cs_p->rq_p);
    }

    free(cs_p);
  }
}

int enterCriticalSection(CriticalSection *cs_p)
{
  void *res_p = NULL;

  assert(cs_p != NULL && cs_p->rq_p != NULL);

  return requestResource(cs_p->rq_p, RQ_REQUEST_WRITE, &res_p);
}

int timedEnterCriticalSection(CriticalSection *cs_p, int ms)
{
  void *res_p = NULL;

  assert(cs_p != NULL && cs_p->rq_p != NULL);

  return timedRequestResource(cs_p->rq_p, RQ_REQUEST_WRITE, &res_p);
}

void exitCriticalSection(CriticalSection *cs_p)
{
  assert(cs_p != NULL && cs_p->rq_p != NULL);

  releaseResource(cs_p->rq_p);
}

void csWakeUpAllWaitingThread(CriticalSection *cs_p)
{
  assert(cs_p != NULL && cs_p->rq_p != NULL);

  rqWakeUpAllWaitingThread(cs_p->rq_p);
}
