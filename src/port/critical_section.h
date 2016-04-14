/*
 * critical_section.h
 *
 *  Created on: Apr 1, 2016
 *      Author: duping
 */

#include "resource_queue.h"

#ifndef CRITICAL_SECTION_H_
#define CRITICAL_SECTION_H_

#define destroyCriticalSectionAndSetNull(cs_p) \
  do { \
    destroyCriticalSection(cs_p); \
    cs_p = NULL; \
  } while(0)

typedef struct critical_section CriticalSection;

struct critical_section {
  ResourceQueue *rq_p;
};

/*
 * Critical Section is implemented internally by resource queue.
 * Call initResourceQueueEnv at the start of the program first.
 */
CriticalSection *createCriticalSection(const char *name_p);
void destroyCriticalSection(CriticalSection *cs_p);

int enterCriticalSection(CriticalSection *cs_p);
int timedEnterCriticalSection(CriticalSection *cs_p, int ms);
void exitCriticalSection(CriticalSection *cs_p);

void csWakeUpAllWaitingThread(CriticalSection *cs_p);

#endif /* CRITICAL_SECTION_H_ */
