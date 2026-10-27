#include "mr-types.h"

void *master(void *args);
void *mapper(void *args);
void *grouper(void *args);
void *reducer(void *args);
void init_partitions(int num_reducers);
void init_mappers(CONTEXT_T *ctx);
void init_reducers(CONTEXT_T *ctx);

CONTEXT_T *ctx = NULL;
PARTITION_T **partitions = NULL;
MAP_WORKER_T **map_workers = NULL;
REDUCE_WORKER_T **reduce_workers = NULL;
GROUPER_RESULT_T **group_results = NULL;
int input_n = 0;
char **input_filenames = NULL;

void *master(void *_) {
	init_mappers(ctx);
	for (int i = 0; i < ctx->n_mappers; i++) {
		pthread_join(map_workers[i]->th, NULL);
	}

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
	for (int i = 0; i < ctx->n_reducers; i++) {
		pthread_join(reduce_workers[i]->th, NULL);
	}

	return NULL;
}

void *mapper(void *args) {
	while (1) {
		pthread_mutex_lock(&ctx->map_files_m);
		if (ctx->map_files_i > ctx->map_files_count - 1) {
			pthread_mutex_unlock(&ctx->map_files_m);
			break;
		}
		int i = ctx->map_files_i++;
		char *input_filename = strdup(input_filenames[i]);
		pthread_mutex_unlock(&ctx->map_files_m);

		ctx->mapper_func(input_filename);
		free(input_filename);
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

void *reducer(void *args) {
	REDUCE_WORKER_T *w = args;

	GROUPER_RESULT_T *gr = group_results[w->partition_i];
	for (unsigned long i = 0; i < gr->groups_count; i++) {
		GROUP_T *g = gr->groups[i];
		ctx->reducer_func(g->key, ctx->getter_func, w->partition_i);
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

	input_n = argc - 1;
	input_filenames = argv + 1;

	ctx = malloc(sizeof(CONTEXT_T));
	ctx->n_mappers = num_mappers;
	ctx->n_reducers = num_reducers;
	ctx->mapper_func = map;
	ctx->reducer_func = reduce;
	ctx->getter_func = get_next;
	ctx->map_files_count = input_n;
	ctx->map_files_i = 0;
	pthread_mutex_init(&ctx->map_files_m, NULL);
	pthread_mutex_init(&ctx->ctx_m, NULL);

	init_partitions(num_reducers);

	pthread_t master_th;
	MASTER_WORKER_T *m = malloc(sizeof(MASTER_WORKER_T));
	m->status = IDLE;
	pthread_create(&master_th, NULL, master, m);
	m->th = master_th;

	pthread_join(m->th, NULL);
}

void init_partitions(int num_reducers) {
	static bool _setup = false;
	if (_setup) {
		return;
	} else {
		_setup = true;
	}

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

void init_mappers(CONTEXT_T *ctx) {
	static bool _setup = false;
	if (_setup) {
		return;
	} else {
		_setup = true;
	}

	int num_mappers = ctx->n_mappers;
	map_workers = malloc(sizeof(void *) * num_mappers);
	for (int i = 0; i < num_mappers; i++) {
		MAP_WORKER_T *w = malloc(sizeof(MAP_WORKER_T));
		w->id = i;

		pthread_create(&w->th, NULL, mapper, w);
		map_workers[i] = w;
	}
}

void init_reducers(CONTEXT_T *ctx) {
	static bool _setup = false;
	if (_setup) {
		return;
	} else {
		_setup = true;
	}

	int num_reducers = ctx->n_reducers;
	reduce_workers = malloc(sizeof(void *) * num_reducers);
	for (int i = 0; i < num_reducers; i++) {
		REDUCE_WORKER_T *w = malloc(sizeof(REDUCE_WORKER_T));
		w->id = i;
		w->partition_i = i;

		pthread_create(&w->th, NULL, reducer, w);
		reduce_workers[i] = w;
	}
}
