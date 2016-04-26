/*
 * base_coding_test.c
 *
 *  Created on: Apr 25, 2016
 *      Author: duping
 */

#include <stdio.h>
#include <string.h>

#include "../../common/common.h"
#include "../../base/base_coding.h"

int main(void)
{
  byte *encoded = NULL;
  byte *decoded = NULL;
  int base64_fail_pos = 0;
  int base32_fail_pos = 0;
  int base16_fail_pos = 0;

  // base64 encoding
  encoded = base64_encode("", 0);
  if (strcmp(encoded, "") != 0) {
    base64_fail_pos |= 0x1;
  }
  base_free(encoded);

  encoded = base64_encode("f", 1);
  if (strcmp(encoded, "Zg==") != 0) {
    base64_fail_pos |= 0x2;
  }
  base_free(encoded);

  encoded = base64_encode("fo", 2);
  if (strcmp(encoded, "Zm8=") != 0) {
    base64_fail_pos |= 0x4;
  }
  base_free(encoded);

  encoded = base64_encode("foo", 3);
  if (strcmp(encoded, "Zm9v") != 0) {
    base64_fail_pos |= 0x8;
  }
  base_free(encoded);

  encoded = base64_encode("foob", 4);
  if (strcmp(encoded, "Zm9vYg==") != 0) {
    base64_fail_pos |= 0x10;
  }
  base_free(encoded);

  encoded = base64_encode("fooba", 5);
  if (strcmp(encoded, "Zm9vYmE=") != 0) {
    base64_fail_pos |= 0x20;
  }
  base_free(encoded);

  encoded = base64_encode("foobar", 6);
  if (strcmp(encoded, "Zm9vYmFy") != 0) {
    base64_fail_pos |= 0x40;
  }
  base_free(encoded);

  // base64 decoding
  encoded = base64_decode("", 0);
  if (strcmp(encoded, "") != 0) {
    base64_fail_pos |= 0x80;
  }
  base_free(encoded);

  encoded = base64_decode("Zg==", 4);
  if (strcmp(encoded, "f") != 0) {
    base64_fail_pos |= 0x100;
  }
  base_free(encoded);

  encoded = base64_decode("Zm8=", 4);
  if (strcmp(encoded, "fo") != 0) {
    base64_fail_pos |= 0x200;
  }
  base_free(encoded);

  encoded = base64_decode("Zm9v", 4);
  if (strcmp(encoded, "foo") != 0) {
    base64_fail_pos |= 0x400;
  }
  base_free(encoded);

  encoded = base64_decode("Zm9vYg==", 8);
  if (strcmp(encoded, "foob") != 0) {
    base64_fail_pos |= 0x800;
  }
  base_free(encoded);

  encoded = base64_decode("Zm9vYmE=", 8);
  if (strcmp(encoded, "fooba") != 0) {
    base64_fail_pos |= 0x1000;
  }
  base_free(encoded);

  encoded = base64_decode("Zm9vYmFy", 8);
  if (strcmp(encoded, "foobar") != 0) {
    base64_fail_pos |= 0x2000;
  }
  base_free(encoded);

  // base32 encoding
  encoded = base32_encode("", 0);
  if (strcmp(encoded, "") != 0) {
    base32_fail_pos |= 0x1;
  }
  base_free(encoded);

  encoded = base32_encode("f", 1);
  if (strcmp(encoded, "MY======") != 0) {
    base32_fail_pos |= 0x2;
  }
  base_free(encoded);

  encoded = base32_encode("fo", 2);
  if (strcmp(encoded, "MZXQ====") != 0) {
    base32_fail_pos |= 0x4;
  }
  base_free(encoded);

  encoded = base32_encode("foo", 3);
  if (strcmp(encoded, "MZXW6===") != 0) {
    base32_fail_pos |= 0x8;
  }
  base_free(encoded);

  encoded = base32_encode("foob", 4);
  if (strcmp(encoded, "MZXW6YQ=") != 0) {
    base32_fail_pos |= 0x10;
  }
  base_free(encoded);

  encoded = base32_encode("fooba", 5);
  if (strcmp(encoded, "MZXW6YTB") != 0) {
    base32_fail_pos |= 0x20;
  }
  base_free(encoded);

  encoded = base32_encode("foobar", 6);
  if (strcmp(encoded, "MZXW6YTBOI======") != 0) {
    base32_fail_pos |= 0x40;
  }
  base_free(encoded);

  //base32 decoding
  encoded = base32_decode("", 0);
  if (strcmp(encoded, "") != 0) {
    base32_fail_pos |= 0x80;
  }
  base_free(encoded);

  encoded = base32_decode("MY======", 8);
  if (strcmp(encoded, "f") != 0) {
    base32_fail_pos |= 0x100;
  }
  base_free(encoded);

  encoded = base32_decode("MZXQ====", 8);
  if (strcmp(encoded, "fo") != 0) {
    base32_fail_pos |= 0x200;
  }
  base_free(encoded);

  encoded = base32_decode("MZXW6===", 8);
  if (strcmp(encoded, "foo") != 0) {
    base32_fail_pos |= 0x400;
  }
  base_free(encoded);

  encoded = base32_decode("MZXW6YQ=", 8);
  if (strcmp(encoded, "foob") != 0) {
    base32_fail_pos |= 0x800;
  }
  base_free(encoded);

  encoded = base32_decode("MZXW6YTB", 8);
  if (strcmp(encoded, "fooba") != 0) {
    base32_fail_pos |= 0x1000;
  }
  base_free(encoded);

  encoded = base32_decode("MZXW6YTBOI======", 16);
  if (strcmp(encoded, "foobar") != 0) {
    base32_fail_pos |= 0x2000;
  }
  base_free(encoded);

  // base16 encoding
  encoded = base16_encode("", 0);
  if (strcmp(encoded, "") != 0) {
    base16_fail_pos |= 0x1;
  }
  base_free(encoded);

  encoded = base16_encode("f", 1);
  if (strcmp(encoded, "66") != 0) {
    base16_fail_pos |= 0x2;
  }
  base_free(encoded);

  encoded = base16_encode("fo", 2);
  if (strcmp(encoded, "666F") != 0) {
    base16_fail_pos |= 0x4;
  }
  base_free(encoded);

  encoded = base16_encode("foo", 3);
  if (strcmp(encoded, "666F6F") != 0) {
    base16_fail_pos |= 0x8;
  }
  base_free(encoded);

  encoded = base16_encode("foob", 4);
  if (strcmp(encoded, "666F6F62") != 0) {
    base16_fail_pos |= 0x10;
  }
  base_free(encoded);

  encoded = base16_encode("fooba", 5);
  if (strcmp(encoded, "666F6F6261") != 0) {
    base16_fail_pos |= 0x20;
  }
  base_free(encoded);

  encoded = base16_encode("foobar", 6);
  if (strcmp(encoded, "666F6F626172") != 0) {
    base16_fail_pos |= 0x40;
  }
  base_free(encoded);

  // base16 decoding
  encoded = base16_decode("", 0);
  if (strcmp(encoded, "") != 0) {
    base16_fail_pos |= 0x80;
  }
  base_free(encoded);

  encoded = base16_decode("66", 2);
  if (strcmp(encoded, "f") != 0) {
    base16_fail_pos |= 0x100;
  }
  base_free(encoded);

  encoded = base16_decode("666F", 4);
  if (strcmp(encoded, "fo") != 0) {
    base16_fail_pos |= 0x200;
  }
  base_free(encoded);

  encoded = base16_decode("666F6F", 6);
  if (strcmp(encoded, "foo") != 0) {
    base16_fail_pos |= 0x400;
  }
  base_free(encoded);

  encoded = base16_decode("666F6F62", 8);
  if (strcmp(encoded, "foob") != 0) {
    base16_fail_pos |= 0x800;
  }
  base_free(encoded);

  encoded = base16_decode("666F6F6261", 10);
  if (strcmp(encoded, "fooba") != 0) {
    base16_fail_pos |= 0x1000;
  }
  base_free(encoded);

  encoded = base16_decode("666F6F626172", 12);
  if (strcmp(encoded, "foobar") != 0) {
    base16_fail_pos |= 0x2000;
  }
  base_free(encoded);

  if (base64_fail_pos == 0) {
    printf("base_coding_test base64 succeeds.\n");
  } else {
    printf("base_coding_test base64 fails. 0x%x \n", base64_fail_pos);
  }

  if (base32_fail_pos == 0) {
    printf("base_coding_test base32 succeeds.\n");
  } else {
    printf("base_coding_test base32 fails. 0x%x \n", base32_fail_pos);
  }

  if (base16_fail_pos == 0) {
    printf("base_coding_test base16 succeeds.\n");
  } else {
    printf("base_coding_test base16 fails. 0x%x \n", base16_fail_pos);
  }

  return 0;
}
