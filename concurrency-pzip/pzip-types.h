#ifndef __PZIP_TYPES__
#define __PZIP_TYPES__

#define MAX_BUF 1024 * 1024 * 1024		 // 1GB
#define BATCH_BUF_SIZE 100 * 1024 * 1024 // 100MB
#define SLOT_INIT_SIZE 1024 * 1024

#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct SLOT {
	char *buf;
	long buf_size;
} SLOT_T;

#endif
