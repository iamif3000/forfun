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

const char *getWorkingDir();
const char *getConfDir();
const char *getDataDir();

#endif /* ENV_H_ */
