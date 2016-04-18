/*
 * os_file_test.c
 *
 *  Created on: Mar 16, 2016
 *      Author: duping
 */

#include <stdio.h>

#include "../../common/error.h"
#include "../../port/os_file.h"

#define FILE_SIZE 0x100000

int main(void)
{
  int error = 0;
  const char file_name[] = "/tmp/os_file_test";
  String *str = NULL;
  int fd;

  (void)remove(file_name);
  fd = createFile(file_name, FILE_SIZE);
  if (fd < 0) {
    error = 1;
    goto end;
  }

  fd = closeFile(fd);
  if (fd < 0) {
    error = 2;
    goto end;
  }

  fd = openFile(file_name);
  if (fd < 0) {
    error = 3;
    goto end;
  }

  for (int i = 0; i < (FILE_SIZE >> 2); ++i) {
    error = writeFileToOffset(fd, (byte*)&i, sizeof(i), i << 2);
    if (error != NO_ERROR) {
      error = 4;
      goto end;
    }
  }

  int j;
  for (int i = 0; i < (FILE_SIZE >> 2); ++i) {
    error = readFileFromOffset(fd, (byte*)&j, sizeof(i), i << 2);
    if (error != NO_ERROR) {
      error = 5;
      goto end;
    }

    if (i != j) {
      error = 6;
      goto end;
    }

    printf("%d ", j);
    if (i % 80 == 0) {
      putchar('\n');
    }
  }

  putchar('\n');

end:

  if (fd > 0) {
    (void)closeFile(fd);
  }

  removeFile(file_name);

  if (error  == NO_ERROR) {
    printf("os_file_test successes!\n");
  } else {
    printf("os_file_test fails!\n");
  }

  return error;
}
