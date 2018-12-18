/*
 * improve performance, skip unnecessary write
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uchar;

static const uchar ENCODE_TAB[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const uchar PAD = '=';

typedef struct {
    uchar b1_l2:2;
    uchar b1_h6:6;
    uchar b2_l4:4;
    uchar b2_h4:4;
    uchar b3_l6:6;
    uchar b3_h2:2;
} B64_Bits;

uchar *b64_encode(const uchar *in, int len) {
    if (in == NULL) {
        return NULL;
    }

    int out_len = (len + 2)/3 << 2 + 1;
    uchar *out = (uchar*)calloc(out_len, sizeof(uchar));
    if (out == NULL) {
        return NULL;
    }

    B64_Bits *bits;
    int i = 0, j = 0;
    for (; i < len; i += 3) {
        bits = (B64_Bits*)(in + i);
        out[j++] = ENCODE_TAB[bits->b1_h6];
        out[j++] = ENCODE_TAB[bits->b1_l2 << 4 | bits->b2_h4];
        out[j++] = ENCODE_TAB[bits->b2_l4 << 2 | bits->b3_h2];
        out[j++] = ENCODE_TAB[bits->b3_l6];
    }

    if (len < i) {
        out[j - 1] = PAD;
        if (len + 1 < i) {
            out[j - 2] = PAD;
        }
    }

    return out;
}

int decode_uchar(const uchar c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    } else if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    } else if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    } else if (c == '+') {
        return 62;
    } else if (c == '/') {
        return 63;
    } else if (c == PAD) {
        return 64;
    }

    return -1;
}

int decode_group(const uchar *in, uchar *out) {
#define DECODE_AND_CHECK(idx) \
    do { \
        decoded = decode_uchar(*(in + idx)); \
        if (decoded < 0 || decoded > 63) { \
            return decoded; \
        } \
    } while(0)

    int decoded;

    B64_Bits *bits = (B64_Bits*)out;

    DECODE_AND_CHECK(0);
    bits->b1_h6 = (uchar)decoded;

    DECODE_AND_CHECK(1);
    bits->b1_l2 = (uchar)decoded  >> 4;
    bits->b2_h4 = (uchar)decoded & 0x0F;

    DECODE_AND_CHECK(2);
    bits->b2_l4 = (uchar)decoded >> 2;
    bits->b3_h2 = (uchar)decoded & 0x03;

    DECODE_AND_CHECK(3);
    bits->b3_l6 = (uchar)decoded;

    return 0;

#undef DECODE_AND_CHECK
}

uchar *b64_decode(const uchar *in, int len) {
    if (len & 0x03 != 0) {
        return NULL;
    }

    int out_len = (len >> 2) * 3 + 1;
    uchar *out = (uchar*)calloc(out_len, sizeof(uchar));
    if (out == NULL) {
        return out;
    }

    int i = 0, j = 0, err;
    for(; i < len; i += 4, j += 3) {
        err = decode_group(in + i, out + j);
        if (err > 63) {
            break;
        } else if (err < 0) {
            free(out);
            return NULL;
        }
    }

    return out;
}

void test_helper(const uchar *in) {
    int len = 0;

    if (in != NULL) {
        len = strlen(in);
    }

    uchar *out1 = b64_encode(in, len);
    if (out1 != NULL) {
        uchar *out2 = b64_decode(out1, strlen(out1));
        if (out2 != NULL) {
            printf("%s|%s|%s|\n", in, out1, out2);
        } else {
            printf("%s|%s\n", in, out1);
        }
        free(out2);
    }

    free(out1);
}

int main(void) {
    test_helper(NULL);
    test_helper("");
    test_helper("f");
    test_helper("fo");
    test_helper("foo");
    test_helper("foob");
    test_helper("fooba");
    test_helper("foobar");

    return 0; 
}

