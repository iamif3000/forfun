#include <iostream>
#include <cstdlib>
#include <cstring>

#define N 10
#define FREE_AND_INIT(p)	\
	{						\
		free((p));			\
		(p) = nullptr;		\
	}

const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789+/";
const char decoding_table[] = { 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, 62, -1, -1, -1, 63, 52, 53,
	54, 55, 56, 57, 58, 59, 60, 61, -1, -1,
	-1, -1, -1, -1, -1,  0,  1,  2,  3,  4,
	 5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
	25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
	29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
	39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, -1, -1, -1, -1, -1
};
const char padding = '=';

char* base64_encoding(const char const* content, const size_t length) {
	if (content == nullptr || length < 1) {
		return nullptr;
	}
	size_t enc_len = (((length + 2) / 3) << 2) + 1;
	char* enc = (char*)malloc(sizeof(char) * enc_len);
	if (enc == nullptr) {
		return nullptr;
	}

	size_t i = 0;
	size_t j = 0;
	for (; i + 3 <= length; i += 3) {
		enc[j++] = encoding_table[content[i] >> 2];
		enc[j++] = encoding_table[((content[i] & 0x03) << 4) | ((content[i+1] & 0xF0) >> 4)];
		enc[j++] = encoding_table[((content[i+1] & 0x0F) << 2) | (content[i+2] >> 6)];
		enc[j++] = encoding_table[content[i+2] & 0x03F];
	}

	// 1 byte left
	if (i + 1 == length) {
		enc[j++] = encoding_table[content[i] >> 2];
		enc[j++] = encoding_table[(content[i] & 0x03) << 4];
		enc[j++] = padding;
		enc[j++] = padding;
	}
	// 2 bytes left
	if (i + 2 == length) {
		enc[j++] = encoding_table[content[i] >> 2];
		enc[j++] = encoding_table[((content[i] & 0x03) << 4) | ((content[i + 1] & 0xF0) >> 4)];
		enc[j++] = encoding_table[(content[i + 1] & 0x0F) << 2];
		enc[j++] = padding;
	}

	enc[j] = '\0';

	return enc;
}

char* base64_decoding(const char const* content, const size_t length) {
	if (content == nullptr || length < 1 || (length & 0x03LL) != 0) {
		return nullptr;
	}
	size_t dec_len = (length >> 2) * 3 + 1;
	char *dec = (char*)malloc(sizeof(char) * dec_len);
	if (dec == nullptr) {
		return nullptr;
	}

	size_t i = 0;
	size_t j = 0;
	for (; i + 4 < length; i += 4) {
		dec[j++] = (decoding_table[content[i]] << 2) | ((decoding_table[content[i + 1]] & 0x30) >> 4);
		dec[j++] = ((decoding_table[content[i + 1]] & 0x0F) << 4) | ((decoding_table[content[i + 2]] & 0x3C) >> 2);
		dec[j++] = ((decoding_table[content[i + 2]] & 0x03) << 6) | decoding_table[content[i + 3]];
	}

	dec[j++] = (decoding_table[content[i]] << 2) | ((decoding_table[content[i + 1]] & 0x30) >> 4);

	if (content[i + 2] == padding) {
		dec[j++] = ((decoding_table[content[i + 1]] & 0x0F) << 4);
	}
	else if (content[i + 3] == padding) {
		dec[j++] = ((decoding_table[content[i + 1]] & 0x0F) << 4) | ((decoding_table[content[i + 2]] & 0x3C) >> 2);
		dec[j++] = ((decoding_table[content[i + 2]] & 0x03) << 6);
	}
	else {
		dec[j++] = ((decoding_table[content[i + 1]] & 0x0F) << 4) | ((decoding_table[content[i + 2]] & 0x3C) >> 2);
		dec[j++] = ((decoding_table[content[i + 2]] & 0x03) << 6) | decoding_table[content[i + 3]];
	}

	dec[j] = '\0';

	return dec;
}

void test_coding(const char* const str) {
	char* enc = nullptr;
	char* dec = nullptr;
	enc = base64_encoding(str, strlen(str));
	dec = base64_decoding(enc, strlen(enc));
	printf("%s %s\n", enc, dec);
	free(enc);
	free(dec);
}

//23
int main23(void) {
	test_coding("f");
	test_coding("fo");
	test_coding("foo");
	test_coding("foob");
	test_coding("fooba");
	test_coding("foobar");

	return 0;
}
