/*
 * env.h
 *
 *  Created on: Apr 22, 2016
 *      Author: duping
 */

#ifndef ENV_H_
#define ENV_H_

int initEnv();
void destroyEnv();

char *getWorkingDir();
char *getConfDir();
char *getDataDir();

#endif /* ENV_H_ */
