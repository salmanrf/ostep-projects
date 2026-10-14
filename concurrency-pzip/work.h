#ifndef __WORK__
#define __WORK__

#include "futils.h"
#include "pzip-types.h"

typedef enum { IDLE, TERMINATED } WORKER_STATUS;

typedef struct WORKER {
	int id;
	SLOT_T *input_slot;
	SLOT_T *output_slot;
	WORKER_STATUS status;
	bool input_ready;
	bool output_ready;
	pthread_mutex_t job_in_l;
	pthread_cond_t job_in_cond;
	pthread_cond_t job_out_cond;
	pthread_mutex_t job_out_l;
} WORKER_T;

typedef struct WORK_CONTEXT {
	FILE_METADATA_T **files_meta;
	int n_files;
	int n_threads;
	int total_work;
} WORK_CONTEXT_T;

int master(int files_n, FILE_METADATA_T **files_meta);

#endif