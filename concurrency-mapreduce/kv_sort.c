#include "kv_sort.h"
#include <stdlib.h>
#include <string.h>

// * Merges sorted halves arr[lo, mid) and arr[mid, hi) using tmp as scratch
static void kv_merge(KV_T **arr, KV_T **tmp, int lo, int mid, int hi) {
	int i = lo;
	int j = mid;
	int k = lo;

	while (i < mid && j < hi) {
		// * <= keeps equal keys in their original order (stable)
		if (strcmp(arr[i]->key, arr[j]->key) <= 0) {
			tmp[k++] = arr[i++];
		} else {
			tmp[k++] = arr[j++];
		}
	}
	while (i < mid) {
		tmp[k++] = arr[i++];
	}
	while (j < hi) {
		tmp[k++] = arr[j++];
	}

	memcpy(arr + lo, tmp + lo, (hi - lo) * sizeof(KV_T *));
}

static void kv_merge_sort(KV_T **arr, KV_T **tmp, int lo, int hi) {
	if (hi - lo < 2) {
		return;
	}

	int mid = lo + (hi - lo) / 2;
	kv_merge_sort(arr, tmp, lo, mid);
	kv_merge_sort(arr, tmp, mid, hi);
	kv_merge(arr, tmp, lo, mid, hi);
}

// * Sorts arr in place by key (strcmp order), stable
// * Returns 0 on success, -1 on allocation failure
int kv_sort(KV_T **arr, int n) {
	if (n < 2) {
		return 0;
	}

	KV_T **tmp = malloc(n * sizeof(KV_T *));
	if (tmp == NULL) {
		return -1;
	}

	kv_merge_sort(arr, tmp, 0, n);

	free(tmp);
	return 0;
}
