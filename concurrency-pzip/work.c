#include "work.h"
#include "compress.h"
#include "futils.h"

WORKER_T *init_worker(int id);
void *worker(void *args);
int get_num_threads_to_create();
long get_total_work(int files_n, FILE_METADATA_T **files_meta);
void clear_mappings(int files_n, FILE_METADATA_T **files_meta);
long get_work(WORK_CONTEXT_T ctx, char *buf, int req_size);
long start_batch(WORK_CONTEXT_T ctx, char **ret, int max_size);

int master(int n_files, FILE_METADATA_T **files_meta) {
	// int n_threads = get_num_threads_to_create();
	int n_threads = 2;
	long total_work = get_total_work(n_files, files_meta);

	printf("Using %d threads\n", n_threads);
	printf("Compressing from %d files with total of %ld bytes\n", n_files,
		   total_work);

	WORKER_T **workers = malloc(sizeof(WORKER_T *) * n_threads);
	pthread_t *threads = malloc(sizeof(pthread_t *) * n_threads);
	for (int i = 0; i < n_threads; i++) {
		workers[i] = init_worker(i);

		int stat = pthread_create(&threads[i], NULL, worker, workers[i]);
		if (stat < 0) {
			perror("pthread_create");
			return 1;
		}
	}

	WORK_CONTEXT_T ctx;
	ctx.files_meta = files_meta;
	ctx.n_files = n_files;
	ctx.n_threads = n_threads;

	int out_fd = open("output.txt", O_CREAT | O_RDWR | O_TRUNC,
					  S_IRUSR | S_IWUSR | S_IROTH);
	if (out_fd < 0) {
		perror("unable to open output file");
		return 1;
	}

	char *batch_buf = NULL;
	long batch_size = 0;

	long processed = 0;
	while (processed < total_work) {
		long remaining = total_work - processed;
		if (remaining < BATCH_BUF_SIZE) {
			batch_size = start_batch(ctx, &batch_buf, remaining);
		} else {
			batch_size = start_batch(ctx, &batch_buf, BATCH_BUF_SIZE);
		}
		printf("Batch initialized with buffer size: %ld\n", batch_size);

		long offset = 0;
		long workload = batch_size / n_threads;
		long rem = batch_size % n_threads;
		for (int i = 0; i < n_threads; i++) {
			WORKER_T *w = workers[i];

			if (i == n_threads - 1) {
				workload += rem;
			}
			w->input_slot = malloc(sizeof(SLOT_T));
			w->input_slot->buf = batch_buf + offset;
			w->input_slot->buf_size = workload;
			w->output_slot = malloc(sizeof(SLOT_T));
			w->output_slot->buf = malloc(workload);
			w->output_slot->buf_size = workload;
			w->input_ready = true;

			offset += workload;

			pthread_cond_signal(&w->job_in_cond);
		}

		for (int i = 0; i < n_threads; i++) {
			WORKER_T *w = workers[i];
			pthread_mutex_lock(&w->job_out_l);
			while (!w->output_ready) {
				pthread_cond_wait(&w->job_out_cond, &w->job_out_l);
			}
			pthread_mutex_unlock(&w->job_out_l);

			printf("worker %d finished with input buf size %ld and output buf "
				   "size %ld \n",
				   w->id, w->input_slot->buf_size, w->output_slot->buf_size);

			/*	TODO: Add merging step between the output slots before writing
			   out results: Example: Slots: [100a, 20a]

				Expected output:
				120a

				Actual output:
				100a20a
			*/
			int stat =
				write(out_fd, w->output_slot->buf, w->output_slot->buf_size);
			if (stat < 0) {
				perror("failed to write to output file");
				return 1;
			}

			free(w->input_slot);
			free(w->output_slot);

			w->input_slot = NULL;
			w->output_slot = NULL;
			w->output_ready = false;
		}

		free(batch_buf);

		processed += batch_size;

		printf("Compressed %ld bytes\n", processed);
		clear_mappings(ctx.n_files, ctx.files_meta);
	}

	close(out_fd);

	/*	Set status to terminated and signal, so upon waking up, the worker will
		quit gracefully
	*/
	for (int i = 0; i < n_threads; i++) {
		WORKER_T *w = workers[i];
		w->input_ready = true;
		w->status = TERMINATED;
		pthread_cond_signal(&w->job_in_cond);
	}

	return 0;
}

long start_batch(WORK_CONTEXT_T ctx, char **ret, int max_size) {
	// * Single buffer for containing all mmapped file contents
	char *buf = malloc(max_size);

	long written = get_work(ctx, buf, max_size);
	if (written <= 0) {
		fprintf(stderr, "unable to allocate work\n");
	}

	*ret = buf;

	return written;
}

long get_work(WORK_CONTEXT_T ctx, char *buf, int req_size) {
	char *start = buf;
	int offset = 0;

	FILE_METADATA_T **files_meta = ctx.files_meta;
	int n_files = ctx.n_files;

	long written = 0;
	while (written < req_size) {
		for (int i = 0; i < n_files; i++) {
			FILE_METADATA_T *f = files_meta[i];
			if (f->processed) {
				continue;
			}

			/* Example, requested size = 100MB
				0: file 0 opened, size = 10MB, processed = 0, remaining = 10MB
					mmap 10MB into buf[0:10MB]
				1: file 1 opened, size = 50MB, processed = 0, remaining = 50MB
					mmap 50MB into buf[10MB:60MB]
				1: file 3 opened, size = 200MB, processed = 120MB, remaining =
			   80MB mmap 40MB into buf[60MB:100MB]
			*/

			long remaining = f->stat->st_size - f->processed;
			if (written + remaining > req_size) {
				remaining = req_size - written;
			}
			char *location = start + offset;

			// TODO: See `man mmap` regarding offset being a multiple of page
			// size
			void *mapped = NULL;
			if ((mapped = mmap(location, remaining, PROT_READ | PROT_WRITE,
							   MAP_PRIVATE | MAP_FIXED, f->fd, f->offset)) ==
				MAP_FAILED) {
				perror("unable to map file content");
				return -1;
			}
			f->curr_mapping = mapped;
			f->curr_mapping_size = remaining;
			f->offset += remaining;

			offset += remaining;
			written += remaining;

			if (f->offset >= f->stat->st_size) {
				f->processed = 1;
			}

			/*
				Prevent other files to be allocated before
				the current one is fully processed
			*/
			break;
		}
	}

	return written;
}

void *worker(void *args) {
	WORKER_T *worker = (WORKER_T *)args;

	while (worker->status != TERMINATED) {
		printf("Worker %d: waiting for job\n", worker->id);
		pthread_mutex_lock(&worker->job_in_l);
		while (!worker->input_ready) {
			pthread_cond_wait(&worker->job_in_cond, &worker->job_in_l);
		}
		worker->input_ready = false;
		pthread_mutex_unlock(&worker->job_in_l);
		if (worker->status == TERMINATED) {
			return NULL;
		}
		printf("Worker %d: got workload of %ld bytes\n", worker->id,
			   worker->input_slot->buf_size);

		compress_rle(worker->input_slot, worker->output_slot);
		worker->output_ready = true;
		pthread_cond_signal(&worker->job_out_cond);
	}

	return NULL;
}

WORKER_T *init_worker(int id) {
	WORKER_T *worker = malloc(sizeof(WORKER_T));
	worker->id = id;
	worker->status = IDLE;
	worker->input_slot = NULL;
	worker->input_ready = false;
	worker->output_ready = false;
	pthread_cond_init(&worker->job_in_cond, NULL);
	pthread_mutex_init(&worker->job_in_l, NULL);
	pthread_cond_init(&worker->job_out_cond, NULL);
	pthread_mutex_init(&worker->job_out_l, NULL);

	return worker;
}

long get_total_work(int files_n, FILE_METADATA_T **files_meta) {
	long tot = 0;

	for (int i = 0; i < files_n; i++) {
		tot += files_meta[i]->stat->st_size;
	}

	return tot;
}

int get_num_threads_to_create() {
	long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	if (nprocs < 0) {
		perror("sysconfg");
	}

	return nprocs;
}

void clear_mappings(int files_n, FILE_METADATA_T **files_meta) {
	for (int i = 0; i < files_n; i++) {
		FILE_METADATA_T *f = files_meta[i];
		munmap(f->curr_mapping, f->curr_mapping_size);
	}
}