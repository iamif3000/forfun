/*
 * os_file.h
 *
 *  Created on: Mar 16, 2016
 *      Author: duping
 */

#include "../common/type.h"
#include "../base/base_string.h"

#ifndef OS_FILE_H_
#define OS_FILE_H_

int createFile(const String *path, count_t size);
int removeFile(const String *path);

int openFile(const String *path);
int closeFile(const int fd);

int readFile(const int fd, byte *bytes, const count_t count);
int readFileFromOffset(const int fd, byte *bytes, const count_t count, const offset_t offset);
int writeFile(const int fd, const byte *bytes, const count_t count);
int writeFileToOffset(const int fd, const byte *bytes, const count_t count, const offset_t offset);

offset_t seekFile(const int fd, const offset_t offset, const int whence);
offset_t posFile(const int fd);

#endif /* OS_FILE_H_ */
