#include "mr-types.h"

void *master(void *args);
void *mapper(void *args);
void *reducer(void *args);
void init_partitions(int num_reducers);
void init_workers(CONTEXT_T *ctx, int num_mappers);

CONTEXT_T *ctx = NULL;
PARTITION_T **partitions = NULL;
MAP_WORKER_T **map_workers = NULL;
int input_n = 0;
char **input_filenames = NULL;

void *master(void *args) {
	MASTER_WORKER_T *m = args;

	for (int i = 0; i < input_n; i++) {
		pthread_mutex_lock(&ctx->mapper_q_m);
		while (queue_is_empty(ctx->mapper_q)) {
			pthread_cond_wait(&ctx->mapper_q_cond, &ctx->mapper_q_m);
		}
		MAP_WORKER_T *w = queue_dequeue(ctx->mapper_q);
		printf("Got map worker %d\n", w->id);
		w->input_filename = input_filenames[i];
		w->input_ready = true;
		pthread_mutex_unlock(&ctx->mapper_q_m);

		pthread_cond_signal(&w->input_cond);
	}

	while (m->status != TERMINATED) {
		pthread_mutex_lock(&ctx->map_ready_m);
		while (ctx->finished_files_count < ctx->input_files_count) {
			pthread_cond_wait(&ctx->map_ready_cond, &ctx->map_ready_m);
			ctx->finished_files_count += 1;
		}
		printf("Map step finished\n");
		pthread_mutex_unlock(&ctx->map_ready_m);
		break;
	}

	for (int i = 0; i < ctx->n_mappers; i++) {
		MAP_WORKER_T *mp = map_workers[i];
		mp->status = TERMINATED;
		mp->input_ready = true;
		pthread_cond_signal(&mp->input_cond);
	}

	printf("Starting intermediate kv pairs sort\n");

	for (int i = 0; i < ctx->n_reducers; i++) {
		PARTITION_T *part = partitions[i];

		int part_data_len = part->curr_i + 1;
		printf("Partition %d has %d items", i, part_data_len);
		kv_sort(part->data, part_data_len);
		for (int j = 0; j < part_data_len; j++) {
			printf("Partition %d, Key: '%s', Value: '%s'\n", i,
				   part->data[j]->key, part->data[j]->val);
		}
	}

	sleep(10);

	return NULL;
}

void *mapper(void *args) {
	MAP_WORKER_T *w = args;

	printf("Map worker %d spawned\n", w->id);

	while (w->status != TERMINATED) {
		pthread_mutex_lock(&w->input_m);
		while (!w->input_ready) {
			pthread_cond_wait(&w->input_cond, &w->input_m);
		}
		if (w->status == TERMINATED) {
			printf("Map worker %d is terminating\n", w->id);
			return NULL;
		}
		w->input_ready = false;
		w->status = BUSY;
		pthread_mutex_unlock(&w->input_m);

		ctx->mapper_func(w->input_filename);
		pthread_mutex_lock(&ctx->mapper_q_m);
		w->status = IDLE;
		queue_enqueue(ctx->mapper_q, w);
		pthread_cond_signal(&ctx->mapper_q_cond);
		pthread_cond_signal(&ctx->map_ready_cond);
		pthread_mutex_unlock(&ctx->mapper_q_m);
	}

	return NULL;
}

void *reducer(void *args) {
	// while () {
	// }

	return NULL;
}

void MR_Emit(char *key, char *value) {
	if (strlen(key) == 0) {
		return;
	}

	if (strcmp(key, " ") == 0) {
		return;
	}

	unsigned long part_key = MR_DefaultHashPartition(key, ctx->n_reducers);

	// static unsigned long n_call = 0;
	// printf("Emit %lu, Key: '%s', Partition: %lu, Value: '%s'\n", n_call, key,
	// 	   part_key, value);
	// n_call++;

	PARTITION_T *part = partitions[part_key];
	pthread_mutex_lock(&part->part_m);
	part->curr_i += 1;
	if (part->curr_i >= part->end_i) {
		part->max_size += sizeof(void *) * MAX_PARTITION_SECT_SIZE;
		part->data = realloc(part->data, part->max_size);
		if (part->data == NULL) {
			perror("unable to grow partition storage");
			exit(1);
		}
		part->end_i += MAX_PARTITION_SECT_SIZE;
	}
	KV_T *kv = malloc(sizeof(KV_T));
	kv->key = strdup(key);
	kv->val = strdup(value);
	part->data[part->curr_i] = kv;
	pthread_mutex_unlock(&part->part_m);
}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
	unsigned long hash = 5381;
	int c;
	while ((c = *key++) != '\0') {
		hash = hash * 33 + c;
	}
	return hash % num_partitions;
}

void MR_Run(int argc, char *argv[], Mapper map, int num_mappers, Reducer reduce,
			int num_reducers, Partitioner partition) {

	if (argc < 2) {
		fprintf(stderr, "Missing input filenames\n");
		exit(1);
	}

	pthread_mutex_t mapper_queue_m;
	pthread_cond_t mapper_queue_cond;
	queue_t *mapper_queue = queue_create();
	pthread_mutex_init(&mapper_queue_m, NULL);
	pthread_cond_init(&mapper_queue_cond, NULL);

	input_n = argc - 1;
	input_filenames = argv + 1;

	ctx = malloc(sizeof(CONTEXT_T));
	ctx->mapper_q = mapper_queue;
	ctx->mapper_q_cond = mapper_queue_cond;
	ctx->mapper_q_m = mapper_queue_m;
	ctx->n_mappers = num_mappers;
	ctx->n_reducers = num_reducers;
	ctx->mapper_func = map;
	ctx->reducer_func = reduce;
	ctx->input_files_count = input_n;
	ctx->finished_files_count = 0;
	pthread_cond_init(&ctx->map_ready_cond, NULL);
	pthread_mutex_init(&ctx->map_ready_m, NULL);

	init_partitions(num_reducers);
	init_workers(ctx, num_mappers);

	pthread_t master_th;
	MASTER_WORKER_T *m = malloc(sizeof(MASTER_WORKER_T));
	m->status = IDLE;
	pthread_create(&master_th, NULL, master, m);
	m->th = master_th;

	pthread_cond_signal(&ctx->mapper_q_cond);

	pthread_join(m->th, NULL);
}

void init_partitions(int num_reducers) {
	static bool _setup = false;
	if (_setup) {
		return;
	} else {
		_setup = true;
	}

	printf("Initializing partitions... \n");

	partitions = malloc(sizeof(void *) * num_reducers);
	for (int i = 0; i < num_reducers; i++) {
		PARTITION_T *part = malloc(sizeof(PARTITION_T));
		part->partition_key = i;
		part->max_size = sizeof(void *) * MAX_PARTITION_SECT_SIZE;
		part->curr_i = -1;
		part->end_i = MAX_PARTITION_SECT_SIZE - 1;
		part->data = malloc(part->max_size);
		pthread_mutex_init(&part->part_m, NULL);

		partitions[i] = part;
	}
}

void init_workers(CONTEXT_T *ctx, int num_mappers) {
	static bool _setup = false;
	if (_setup) {
		return;
	} else {
		_setup = true;
	}

	printf("Initializing workers... \n");

	map_workers = malloc(sizeof(void *) * num_mappers);
	for (int i = 0; i < num_mappers; i++) {
		MAP_WORKER_T *w = malloc(sizeof(MAP_WORKER_T));
		w->id = i;
		w->status = IDLE;
		w->input_ready = false;
		pthread_cond_init(&w->input_cond, NULL);
		pthread_mutex_init(&w->input_m, NULL);

		pthread_create(&w->th, NULL, mapper, w);
		map_workers[i] = w;
		queue_enqueue(ctx->mapper_q, w);
	}
}
