#ifndef __FUTILS__
#define __FUTILS__

#include "pzip-types.h"

typedef struct stat FILE_STAT;

typedef struct FILE_METADATA {
	int fd;
	FILE_STAT *stat;
	long offset;
	long processed;			// 0: not finished, 1: fully processed
	void *curr_mapping;		// reference to last mmap call result
	long curr_mapping_size; // size of the last mmap call
} FILE_METADATA_T;

FILE **open_files(int n_path, char **paths);
FILE_METADATA_T **open_files_metadata(int n_path, char **paths);

#endif