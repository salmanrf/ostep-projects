#ifndef __KV_SORT_H__
#define __KV_SORT_H__

typedef struct KV {
	char *key;
	char *val;
} KV_T;

int kv_sort(KV_T **arr, int n);

#endif
