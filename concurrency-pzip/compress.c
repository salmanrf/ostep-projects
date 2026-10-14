#include "compress.h"

int compress_rle(SLOT_T *input_slot, SLOT_T *output_slot) {
	char *input_buf = input_slot->buf;
	long input_size = input_slot->buf_size;

	if (input_size == 0) {
		return 0;
	}

	long x = 1, y = 0;
	long ch = input_buf[0];
	char last_ch = ch;
	long n_ch = 1;
	long n_ch_size = sizeof(long);

	long written = 0;
	char *output_buf = output_slot->buf;
	long output_size = output_slot->buf_size;

	for (; x < input_size && y < output_size; x++) {
		// * Write upon end of consecutive characters, or newline
		if (ch != last_ch) {
			output_buf[y] = n_ch;
			memcpy(output_buf + y, &n_ch, n_ch_size);
			y += n_ch_size;
			written += n_ch_size;
			memcpy(output_buf + y, &last_ch, 1);
			y += 1;
			written += 1;

			n_ch = 1;
			y += written;
		} else {
			n_ch += 1;
		}

		last_ch = ch;
	}

	memcpy(output_buf + y, &n_ch, n_ch_size);
	y += n_ch_size;
	written += n_ch_size;
	memcpy(output_buf + y, &last_ch, 1);
	y += 1;
	written += 1;

	output_slot->buf_size = written;

	return written;
}