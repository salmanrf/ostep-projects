#include "mr-types.h"

void *master(void *args);
void *mapper(void *args);
void *grouper(void *args);
void *reducer(void *args);
void *assigner(void *args);
void init_partitions(int num_reducers);
void init_workers(CONTEXT_T *ctx);
void init_reducers(CONTEXT_T *ctx);

CONTEXT_T *ctx = NULL;
PARTITION_T **partitions = NULL;
MAP_WORKER_T **map_workers = NULL;
REDUCE_WORKER_T **reduce_workers = NULL;
GROUPER_RESULT_T **group_results = NULL;
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
		kv_sort(part->data, part_data_len);
	}

	int partition_count = ctx->n_reducers;

	group_results = malloc(sizeof(void *) * partition_count);
	pthread_t *grouper_ths = malloc(sizeof(pthread_t) * partition_count);
	for (int i = 0; i < partition_count; i++) {
		GROUPER_WORKER_T *args = malloc(sizeof(GROUPER_WORKER_T));
		args->partition_key = i;
		pthread_create(&grouper_ths[i], NULL, grouper, args);
	}
	for (int i = 0; i < partition_count; i++) {
		pthread_join(grouper_ths[i], (void *)&group_results[i]);
	}

	free(partitions);

	init_reducers(ctx);

	ASSIGNER_WORKER_T **assigners = malloc(sizeof(void *) * partition_count);
	for (int i = 0; i < partition_count; i++) {
		ASSIGNER_WORKER_T *args = malloc(sizeof(ASSIGNER_WORKER_T));
		args->partition_key = i;
		args->partition_key_groups = group_results[i];
		assigners[i] = args;

		pthread_create(&args->th, NULL, assigner, args);
	}

	pthread_mutex_lock(&ctx->reduce_ready_m);
	while (queue_size(ctx->reducer_q) < ctx->n_reducers) {
		pthread_cond_wait(&ctx->reduce_ready_cond, &ctx->reduce_ready_m);
	}
	pthread_mutex_unlock(&ctx->reduce_ready_m);

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

void *grouper(void *args) {
	GROUPER_WORKER_T *w = args;

	KV_T **data = partitions[w->partition_key]->data;
	long data_len = partitions[w->partition_key]->curr_i + 1;

	GROUPER_RESULT_T *result = malloc(sizeof(GROUPER_RESULT_T));
	unsigned long result_len = 100;
	GROUP_T **groups = malloc(sizeof(GROUP_T *) * result_len);
	unsigned long group_i = -1;

	char *last_key = "";
	for (int i = 0; i < data_len; i++) {
		KV_T *kv = data[i];

		// * data is sorted, so a key change always starts a new group
		if (strcmp(last_key, kv->key) != 0) {
			group_i += 1;

			if (group_i >= result_len) {
				result_len += 100;
				groups = realloc(groups, sizeof(GROUP_T *) * result_len);
				if (groups == NULL) {
					perror("unable to grow groups storage");
					exit(1);
				}
			}

			GROUP_T *new_group = malloc(sizeof(GROUP_T));
			new_group->key = strdup(kv->key);
			new_group->values = list_create();
			groups[group_i] = new_group;
		}

		GROUP_T *group = groups[group_i];
		list_push_back(group->values, strdup(kv->val));

		last_key = group->key;
	}

	result->groups = groups;
	result->groups_count = group_i + 1;

	return result;
}

char *get_next(char *key, int partition_number) {
	GROUPER_RESULT_T *work_group = group_results[partition_number];

	for (int i = 0; i < work_group->groups_count; i++) {
		if (strcmp(key, work_group->groups[i]->key) == 0) {
			linked_list_t *values = work_group->groups[i]->values;
			void *val = list_pop_front(values);
			return val;
		}
	}

	return NULL;
}

void *assigner(void *args) {
	ASSIGNER_WORKER_T *worker = args;

	for (int i = 0; i < worker->partition_key_groups->groups_count; i++) {
		pthread_mutex_lock(&ctx->reducer_q_m);
		while (queue_is_empty(ctx->reducer_q)) {
			pthread_cond_wait(&ctx->reducer_q_cond, &ctx->reducer_q_m);
		}
		REDUCE_WORKER_T *r = queue_dequeue(ctx->reducer_q);
		pthread_mutex_unlock(&ctx->reducer_q_m);

		pthread_mutex_lock(&r->input_m);
		r->input_key = worker->partition_key_groups->groups[i]->key;
		r->input_ready = true;
		r->partition_number = worker->partition_key;
		pthread_cond_signal(&r->input_cond);
		pthread_mutex_unlock(&r->input_m);
	}

	return NULL;
}

void *reducer(void *args) {
	REDUCE_WORKER_T *w = args;

	printf("Reduce worker %d spawned\n", w->id);

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

		ctx->reducer_func(w->input_key, ctx->getter_func, w->partition_number);
		pthread_mutex_lock(&ctx->reducer_q_m);
		w->status = IDLE;
		queue_enqueue(ctx->reducer_q, w);
		pthread_cond_signal(&ctx->reducer_q_cond);
		pthread_cond_signal(&ctx->reduce_ready_cond);
		pthread_mutex_unlock(&ctx->reducer_q_m);
	}

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

	pthread_mutex_t reducer_queue_m;
	pthread_cond_t reducer_queue_cond;
	queue_t *reducer_queue = queue_create();
	pthread_mutex_init(&reducer_queue_m, NULL);
	pthread_cond_init(&reducer_queue_cond, NULL);

	input_n = argc - 1;
	input_filenames = argv + 1;

	ctx = malloc(sizeof(CONTEXT_T));
	ctx->mapper_q = mapper_queue;
	ctx->mapper_q_cond = mapper_queue_cond;
	ctx->mapper_q_m = mapper_queue_m;
	ctx->reducer_q = reducer_queue;
	ctx->reducer_q_cond = reducer_queue_cond;
	ctx->reducer_q_m = reducer_queue_m;
	ctx->n_mappers = num_mappers;
	ctx->n_reducers = num_reducers;
	ctx->mapper_func = map;
	ctx->reducer_func = reduce;
	ctx->getter_func = get_next;
	ctx->input_files_count = input_n;
	ctx->finished_files_count = 0;
	pthread_cond_init(&ctx->map_ready_cond, NULL);
	pthread_mutex_init(&ctx->map_ready_m, NULL);
	pthread_cond_init(&ctx->reduce_ready_cond, NULL);
	pthread_mutex_init(&ctx->reduce_ready_m, NULL);
	pthread_mutex_init(&ctx->ctx_m, NULL);

	init_partitions(num_reducers);
	init_workers(ctx);

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

void init_workers(CONTEXT_T *ctx) {
	static bool _setup = false;
	if (_setup) {
		return;
	} else {
		_setup = true;
	}

	printf("Initializing workers... \n");

	int num_mappers = ctx->n_reducers;
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

void init_reducers(CONTEXT_T *ctx) {
	static bool _setup = false;
	if (_setup) {
		return;
	} else {
		_setup = true;
	}

	printf("Initializing reducers... \n");

	int num_reducers = ctx->n_reducers;
	reduce_workers = malloc(sizeof(void *) * num_reducers);
	for (int i = 0; i < num_reducers; i++) {
		REDUCE_WORKER_T *w = malloc(sizeof(REDUCE_WORKER_T));
		w->id = i;
		w->status = IDLE;
		w->input_ready = false;
		pthread_cond_init(&w->input_cond, NULL);
		pthread_mutex_init(&w->input_m, NULL);

		pthread_create(&w->th, NULL, reducer, w);
		reduce_workers[i] = w;
		queue_enqueue(ctx->reducer_q, w);
	}
}
