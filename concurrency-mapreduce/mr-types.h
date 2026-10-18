#ifndef __MR_TYPES_H__
#define __MR_TYPES_H__

#include "kv_sort.h"
#include "linked_list.h"
#include "mapreduce.h"
#include "queue.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PARTITION_SECT_SIZE 100

typedef enum { IDLE, BUSY, TERMINATED } WORKER_STATUS;

// typedef struct PARTITION_SECT {
// 	int max_size;
// 	int curr_i;
// 	KV_T *data;
// 	pthread_mutex_t sect_m;
// } PARTITION_SECT_T;

typedef struct PARTITION {
	// linked_list_t *sections;
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
	bool input_ready;
	char *input_filename;
	WORKER_STATUS status;
	pthread_cond_t input_cond;
	pthread_mutex_t input_m;
	pthread_t th;
} MAP_WORKER_T;

typedef struct REDUCE_WORKER {
	bool input_ready;
	bool output_ready;
	WORKER_STATUS status;
	pthread_cond_t input_cond;
	pthread_mutex_t input_m;
	pthread_t th;
} REDUCE_WORKER_T;

typedef struct CONTEXT {
	queue_t *mapper_q;
	pthread_mutex_t mapper_q_m;
	pthread_mutex_t map_ready_m;
	pthread_cond_t mapper_q_cond;
	pthread_cond_t map_ready_cond;
	int n_mappers;
	int n_reducers;
	Mapper mapper_func;
	Reducer reducer_func;
	int input_files_count;
	int finished_files_count;
} CONTEXT_T;

#endif
