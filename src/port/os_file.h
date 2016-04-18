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

int createFile(const char *path_p, count_t size);
int removeFile(const char *path_p);

int openFile(const char *path_p);
int closeFile(const int fd);

ssize_t readFile(const int fd, byte *bytes, const count_t count);
ssize_t readFileFromOffset(const int fd, byte *bytes, const count_t count, const offset_t offset);
ssize_t writeFile(const int fd, const byte *bytes, const count_t count);
ssize_t writeFileToOffset(const int fd, const byte *bytes, const count_t count, const offset_t offset);

offset_t seekFile(const int fd, const offset_t offset, const int whence);
offset_t posFile(const int fd);

bool canAccessFile(const char *path_p);

#endif /* OS_FILE_H_ */
