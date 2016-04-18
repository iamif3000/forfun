/*
 * os_file.c
 *
 *  Created on: Mar 16, 2016
 *      Author: duping
 */

// TODO : more OS

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "../common/common.h"
#include "os_file.h"

/*
 * return < 0 means error occurred, > 0 means fd
 */
int createFile(const char *path_p, count_t size)
{
  int error = NO_ERROR;
  offset_t ofst = 0;
  int fd;

  assert(path_p != NULL);

  error = open(path_p, O_CREAT | O_EXCL | O_RDWR | O_NONBLOCK, S_IRUSR | S_IWUSR);
  if (error < 0) {
    error = ER_OS_FILE_CREATE_FAIL;
    goto Error;
  }

  // preserve the space
  fd = error;

  if (size > 0) {
    // write 1 byte at the end
    error = writeFileToOffset(fd, "\0", 1, size - 1);
    if (error != NO_ERROR) {
      goto Error;
    }

    // set offset to the start
    ofst = seekFile(fd, 0, SEEK_SET);
    if (ofst < 0) {
      error = (int)ofst;
      goto Error;
    }
  }

  return fd;

Error:

  return error;
}

int removeFile(const char *path_p)
{
  int error = NO_ERROR;

  assert(path_p != NULL);

  error = unlink(path_p);
  if (error < 0) {
    error = ER_OS_FILE_REMOVE_FAIL;
  }

  return error;
}

/*
 * return: >0 fd, <0 error
 */
int openFile(const char *path_p)
{
  int error = NO_ERROR;

  assert(path_p != NULL);

  error = open(path_p, O_RDWR | O_NONBLOCK);
  if (error < 0) {
    error = ER_OS_FILE_OPEN_FAIL;
  }

  return error;
}

int closeFile(const int fd)
{
  int error = NO_ERROR;

  assert(fd > 0);

  error = close(fd);
  if (error < 0) {
    error = ER_OS_FILE_CLOSE_FAIL;
  }

  return error;
}

ssize_t readFile(const int fd, byte *bytes, const count_t count)
{
  ssize_t error = NO_ERROR;
  ssize_t ret, _read = 0;

  assert(fd > 0 && bytes != NULL);

  while(_read < count) {
    ret = read(fd, bytes + _read, count - _read);
    if (ret < 0) {
      switch (errno) {
      case EAGAIN:
      case EINTR:
        continue;
      default:
        error = ER_OS_FILE_READ_FAIL;
        goto Error;
      }
    } else if (ret > 0) {
      _read += ret;
      continue;
    } else {
      // end of file or count is 0
      break;
    }
  }

  error = _read;

Error:

  return error;
}

ssize_t readFileFromOffset(const int fd, byte *bytes, const count_t count, const offset_t offset)
{
  ssize_t error = NO_ERROR;
  offset_t ofst = 0;

  assert(fd > 0 && bytes != NULL);

  ofst = seekFile(fd, offset, SEEK_SET);
  if (ofst < 0) {
    error = (ssize_t)ofst;
    goto Error;
  }

  error = readFile(fd, bytes, count);
  if (error < 0) {
    goto Error;
  }

Error:

  return error;
}

ssize_t writeFile(const int fd, const byte *bytes, const count_t count)
{
  ssize_t error = NO_ERROR;
  ssize_t ret, wrote = 0;

  assert(fd > 0 && bytes != NULL);

  while(wrote < count) {
    ret = write(fd, bytes + wrote, count - wrote);
    if (ret < 0) {
      switch(errno) {
      case EAGAIN:
      case EINTR:
        continue;
      default:
        error = ER_OS_FILE_WRITE_FAIL;
        goto Error;
      }
    } else if (ret > 0) {
      wrote += ret;
    } else {
      // end of file or count is 0
      break;
    }
  }

  error = wrote;

Error:

  return error;
}

ssize_t writeFileToOffset(const int fd, const byte *bytes, const count_t count, const offset_t offset)
{
  ssize_t error = NO_ERROR;
  offset_t ofst = 0;

  assert(fd > 0 && bytes != NULL);

  ofst = seekFile(fd, offset, SEEK_SET);
  if (ofst < 0) {
    error = (ssize_t)ofst;
    goto Error;
  }

  error = writeFile(fd, bytes, count);
  if (error < NO_ERROR) {
    goto Error;
  }

Error:

  return error;
}

/*
 * return : < 0 error, else position after seek
 */
offset_t seekFile(const int fd, const offset_t offset, const int whence)
{
  off_t ofst = NO_ERROR;

  assert(fd > -1);

  ofst = lseek(fd, offset, whence);
  if (ofst < 0) {
    ofst = ER_OS_FILE_SEEK_FAIL;
  }

  return ofst;
}

/*
 * return : < 0 error, else current position
 */
offset_t posFile(const int fd)
{
  off_t ofst = NO_ERROR;

  assert(fd > -1);

  ofst = lseek(fd, 0, SEEK_CUR);
  if (ofst < 0) {
    ofst = ER_OS_FILE_SEEK_FAIL;
  }

  return ofst;
}

bool canAccessFile(const char *path_p)
{
  int error = NO_ERROR;
  struct stat buf;

  assert(path_p != NULL);

  error = stat(path_p, &buf);
  if (error < 0) {
    return false;
  }

  return true;
}

