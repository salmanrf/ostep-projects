#include "futils.h"

FILE **open_files(int n_path, char **paths) {
	FILE **files = (FILE **)malloc(sizeof(FILE *) * n_path);

	for (int i = 0; i < n_path; i++) {
		char *path = paths[i];
		FILE *fp = fopen(path, "r");
		if (fp == NULL) {
			fprintf(stderr, "wzip: unable to open file %s\n", path);
			exit(1);
		}

		files[i] = fp;

		FILE_STAT stat;
		if (fstat(fp->_file, &stat) < 0) {
			perror("fstat:");
			exit(1);
		}
	}

	return files;
}

FILE_METADATA_T **open_files_metadata(int n_path, char **paths) {
	FILE_METADATA_T **metas =
		(FILE_METADATA_T **)malloc(sizeof(FILE_METADATA_T *) * n_path);

	for (int i = 0; i < n_path; i++) {
		char *path = paths[i];
		int fd = open(path, O_RDONLY, 0);
		if (fd < 0) {
			fprintf(stderr, "wzip: unable to open file %s\n", path);
			exit(1);
		}

		FILE_METADATA_T *m = malloc(sizeof(FILE_METADATA_T));
		m->fd = fd;
		m->offset = 0;
		m->processed = 0;

		FILE_STAT *stat = malloc(sizeof(FILE_STAT));
		if (fstat(fd, stat) < 0) {
			perror("fstat:");
			exit(1);
		}
		m->stat = stat;

		if (stat->st_size == 0) {
			close(fd);
			metas[i] = m;
			continue;
		}

		metas[i] = m;
	}

	return metas;
}

void destroy_files_metadata(int n_files, FILE_METADATA_T **files_meta) {
	for (int i = 0; i < n_files; i++) {
		FILE_METADATA_T *f = files_meta[i];
		close(f->fd);
		free(f);
	}
}