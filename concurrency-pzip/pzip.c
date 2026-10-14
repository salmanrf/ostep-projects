#include "futils.h"
#include "work.h"
#include <string.h>
#include <time.h>

int compress_rle(int n_file, FILE **fp, FILE *out);
int getc_from_files(int n_file, FILE **fp);

int get_num_threads_to_create();

int main(int argc, char *argv[]) {
	struct timespec t_start;
	clock_gettime(CLOCK_MONOTONIC, &t_start);

	if (argc < 2) {
		fprintf(stdout, "pzip: file1 [file2 ...]\n");
		exit(1);
	}

	char **paths = argv + 1;
	int n_files = argc - 1;

	FILE_METADATA_T **files_meta = open_files_metadata(n_files, paths);
	int stat = master(n_files, files_meta);
	if (stat != 0) {
		fprintf(stderr, "pzip: terminated with error\n");
		return stat;
	}

	struct timespec t_end;
	clock_gettime(CLOCK_MONOTONIC, &t_end);
	double elapsed = (t_end.tv_sec - t_start.tv_sec) +
					 (t_end.tv_nsec - t_start.tv_nsec) / 1e9;
	fprintf(stderr, "pzip: elapsed %.6f s\n", elapsed);

	return 0;
}
