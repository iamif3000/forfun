#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <ctime>

union OptValue {
	int8_t i8;
	int16_t i16;
	int32_t i32;
	int64_t i64;
	uintptr_t uiptr;
	float f32;
	double f64;
	void* ptr;
};

typedef union OptValue OptValue;
typedef struct SkipListNode SkipListNode;
typedef struct SkipList SkipList;
typedef int (*OptValueCmp)(const OptValue, const OptValue);

int intCmp(const OptValue v1, const OptValue v2) {
	if (v1.i32 > v2.i32) {
		return 1;
	}
	if (v1.i32 < v2.i32) {
		return -1;
	}

	return 0;
}

struct SkipListNode {
	OptValue value;
	SkipListNode** level_next;
	SkipListNode*** level_prev_next;
};

struct SkipList {
	int level;
	SkipListNode** level_ptr;
	SkipListNode** help_ptr;
	OptValueCmp cmp;
};

SkipListNode* createSkipListNode(const OptValue value, const int level) {
	if (level < 1) {
		return nullptr;
	}

	SkipListNode* node = (SkipListNode*)calloc(1, sizeof(SkipListNode));
	if (node != nullptr) {
		node->value = value;
		node->level_next = (SkipListNode**)calloc(level, sizeof(SkipListNode*));
		node->level_prev_next = (SkipListNode***)calloc(level, sizeof(SkipListNode**));
		if (node->level_next == nullptr || node->level_prev_next == nullptr) {
			free(node->level_next);
			free(node->level_prev_next);
			free(node);
			return nullptr;
		}
	}

	return node;
}

void destroySkipListNode(SkipListNode* node) {
	if (node != nullptr) {
		free(node->level_next);
		free(node->level_prev_next);
		free(node);
	}
}

int skipListInit(SkipList* skip_list, const int level, OptValueCmp cmp) {
	if (skip_list == nullptr || level < 1 || cmp == nullptr) {
		return -1;
	}
	skip_list->level = level;
	skip_list->level_ptr = (SkipListNode**)calloc(level, sizeof(SkipListNode*));
	skip_list->help_ptr = (SkipListNode**)calloc(level, sizeof(SkipListNode*));
	if (skip_list->level_ptr == nullptr || skip_list->help_ptr == nullptr) {
		free(skip_list->level_ptr);
		free(skip_list->help_ptr);
		return -1;
	}
	skip_list->cmp = cmp;

	return 0;
}

void skipListClear(SkipList* skip_list) {
	if (skip_list == nullptr) {
		return;
	}
	if (skip_list->level_ptr != nullptr) {
		int last_level_index = skip_list->level - 1;
		SkipListNode* node = skip_list->level_ptr[last_level_index];
		for (SkipListNode* next; node != nullptr; node = next) {
			next = node->level_next[last_level_index];
			destroySkipListNode(node);
		}
		free(skip_list->level_ptr);
		free(skip_list->help_ptr);
		skip_list->level_ptr = nullptr;
		skip_list->level = 0;
		skip_list->cmp = nullptr;
	}
}

SkipListNode* _skipListSearch(SkipList* skip_list, const OptValue value) {
	if (skip_list == nullptr) {
		return nullptr;
	}
	SkipListNode* node = nullptr;
	memset(skip_list->help_ptr, NULL, skip_list->level * sizeof(SkipListNode*));
	for (int i = 0; i < skip_list->level; ++i) {
		if (i == 0 || skip_list->help_ptr[i - 1] == nullptr) {
			node = skip_list->level_ptr[i];
		}
		else {
			node = skip_list->help_ptr[i - 1];
		}
		for (; node != nullptr; node = node->level_next[i]) {
			int cmp_result = skip_list->cmp(value, node->value);
			if (cmp_result == 0) {
				return node;
			}
			else if (cmp_result < 0) {
				// go to next level
				break;
			}
			else { // > 0
				skip_list->help_ptr[i] = node;
			}
		}
	}

	return nullptr;
}

int skipListSearch(SkipList* skip_list, const OptValue value) {
	return _skipListSearch(skip_list, value) != nullptr;
}

double getRD() {
	double r = (double)rand() / RAND_MAX;
	return r;
}

int skipListInsert(SkipList* skip_list, const OptValue value) {
	if (skip_list == nullptr) {
		return -1;
	}
	SkipListNode* node = createSkipListNode(value, skip_list->level);
	if (node == nullptr) {
		return -1;
	}

	int level = skip_list->level;
	SkipListNode* search_ptr = nullptr;
	memset(skip_list->help_ptr, NULL, skip_list->level * sizeof(SkipListNode*));
	for (int i = 0; i < level; ++i) {
		if (i == 0 || skip_list->help_ptr[i - 1] == nullptr) {
			search_ptr = skip_list->level_ptr[i];
		}
		else {
			search_ptr = skip_list->help_ptr[i - 1];
		}

		for (; search_ptr != nullptr; search_ptr = search_ptr->level_next[i]) {
			int cmp_result = skip_list->cmp(value, search_ptr->value);
			if (cmp_result <= 0) {
				// go to next level
				break;
			}
			else {
				// > 0
				skip_list->help_ptr[i] = search_ptr;
			}
		}
	}
	// do insert
	int i = level - 1;
	do {
		if (skip_list->help_ptr[i] == nullptr) {
			node->level_next[i] = skip_list->level_ptr[i];
			if (skip_list->level_ptr[i] != nullptr) {
				skip_list->level_ptr[i]->level_prev_next[i] = &node->level_next[i];
			}
			node->level_prev_next[i] = &skip_list->level_ptr[i];
			skip_list->level_ptr[i] = node;
		}
		else {
			node->level_next[i] = skip_list->help_ptr[i]->level_next[i];
			if (skip_list->help_ptr[i]->level_next[i] != nullptr) {
				skip_list->help_ptr[i]->level_next[i]->level_prev_next[i] = &node->level_next[i];
			}
			node->level_prev_next[i] = &skip_list->help_ptr[i]->level_next[i];
			skip_list->help_ptr[i]->level_next[i] = node;
		}
		if (getRD() < 0.5) {
			break;
		}
	} while (--i >= 0);

	return 0;
}

//24
int main(void) {
#define N 1024
	SkipList list;
	OptValue value[N];

	srand(time(nullptr));

	skipListInit(&list, 16, intCmp);

	for (int i = 0; i < N; ++i) {
		value[i].i32 = rand() % N + 1;
		skipListInsert(&list, value[i]);
	}

	for (int i = 0; i < list.level; ++i) {
		for (SkipListNode* node = list.level_ptr[i]; node != nullptr; node = node->level_next[i]) {
			printf("%2d  ", node->value.i32);
		}
		printf("\n");
	}

	OptValue search_val;
	search_val.i32 = 1024;
	int count = 0;
	for (int i = 0; i < 10000000; ++i) {
		count += skipListSearch(&list, search_val);
		/*for (SkipListNode* node = list.level_ptr[list.level - 1]; node != nullptr; node = node->level_next[list.level - 1]) {
			if (node->value.i32 == search_val.i32) {
				++count;
			}
		}//*/
	}
	printf("%d\n", count);

	skipListClear(&list);

	return 0;
}
