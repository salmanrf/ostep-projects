#ifndef __MR_TYPES_H__
#define __MR_TYPES_H__

#include "kv_sort.h"
#include "linked_list.h"
#include "mapreduce.h"
#include "queue.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PARTITION_SECT_SIZE 100

typedef enum { IDLE, BUSY, TERMINATED } WORKER_STATUS;

typedef struct PARTITION {
	unsigned long partition_key;
	long max_size;
	long end_i;
	long curr_i;
	KV_T **data;
	pthread_mutex_t part_m;
} PARTITION_T;

typedef struct MASTER_WORKER {
	WORKER_STATUS status;
	pthread_t th;
} MASTER_WORKER_T;

typedef struct MAP_WORKER {
	int id;
	pthread_t th;
} MAP_WORKER_T;

typedef struct GROUPER_WORKER {
	int partition_key;
} GROUPER_WORKER_T;

typedef struct GROUP {
	char *key;
	linked_list_t *values;
} GROUP_T;

typedef struct GROUPER_RESULT {
	GROUP_T **groups;
	unsigned long groups_count;
} GROUPER_RESULT_T;

typedef struct ASSIGNER_WORKER {
	int partition_key;
	GROUPER_RESULT_T *partition_key_groups;
	pthread_t th;
} ASSIGNER_WORKER_T;

typedef struct REDUCE_WORKER {
	int id;
	int partition_i;
	pthread_t th;
} REDUCE_WORKER_T;

typedef struct CONTEXT {
	int n_mappers;
	int n_reducers;
	Mapper mapper_func;
	Reducer reducer_func;
	Getter getter_func;
	pthread_mutex_t ctx_m;
	int map_files_count;
	int map_files_i;
	pthread_mutex_t map_files_m;
} CONTEXT_T;

#endif
