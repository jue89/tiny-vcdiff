#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include "vcdiff.h"
#include "vcdiff/state.h"

struct target_stream {
	FILE *file;
	size_t offset;

	size_t log_interval;
	size_t log_last_offset;
	size_t log_delta_written;
};

static void log_stats (struct target_stream *target, bool force) {
	if (target->log_interval && (force || target->log_last_offset + target->log_interval < target->offset)) {
		target->log_last_offset = target->offset;
		fprintf(stderr, "STATS IN=%lukB OUT=%lukB RATIO=1:%lu\n",
			target->log_delta_written / 1024,
			target->offset / 1024,
			(target->offset / target->log_delta_written) + 1);
	}
}

static int _target_write (void *dev, uint8_t *data, size_t offset, size_t len) {
	struct target_stream *target = (struct target_stream *) dev;

	if (target->offset != offset) {
		/* Gapped write not supported! */
		return -ENOTSUP;
	}

	size_t bytes_written = fwrite(data, sizeof(data[0]), len, target->file);
	if (bytes_written != len) {
		return -ENOMEM;
	}

	target->offset += bytes_written;

	log_stats(target, false);

	return 0;
}

static int _target_read (void *dev, uint8_t *dest, size_t offset, size_t len) {
	(void) dev;
	(void) dest;
	(void) offset;
	(void) len;

	/* data written into the pipe is gone! */
	return -ENOTSUP;
}

static const vcdiff_driver_t target_driver = {
	.read = _target_read,
	.write = _target_write
};

static int _source_read (void *dev, uint8_t *dest, size_t offset, size_t len) {
	FILE *source = (FILE *) dev;

	int rc = fseek(source, offset, SEEK_SET);
	if (rc < 0) {
		perror("Cannot seek source");
		return -ESPIPE;
	}

	size_t bytes_read = fread(dest, sizeof(uint8_t), len, source);
	if (bytes_read != len) {
		return -EIO;
	}

	return 0;
}

static const vcdiff_driver_t source_driver = {
	.read = _source_read
};

static int apply_delta(FILE *delta, FILE *source, FILE *target_file, size_t log_interval, vcdiff_log_t inst_log) {
	static vcdiff_t ctx;
	struct target_stream target = {.file = target_file, .log_interval = log_interval};
	uint8_t delta_buf[16 * 1024];
	size_t delta_len;

	vcdiff_init(&ctx);
	vcdiff_set_logger(&ctx, inst_log, NULL);
	vcdiff_set_source_driver(&ctx, &source_driver, (void *) source);
	vcdiff_set_target_driver(&ctx, &target_driver, (void *) &target);

	while ((delta_len = fread(delta_buf, sizeof(delta_buf[0]), sizeof(delta_buf), delta))) {
		target.log_delta_written += delta_len;
		int rc = vcdiff_apply_delta(&ctx, delta_buf, delta_len);
		if (rc < 0) {
			fprintf(stderr, "Error while applying delta: %s\n", vcdiff_error_str(&ctx));
			return rc;
		}
	}

	log_stats(&target, true);

	return 0;
}

static void usage (void) {
	fprintf(stderr, "Usage: vcdiff-decode [-i] [-s <interval>] source_path\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -i              Enable instruction log\n");
	fprintf(stderr, "  -s <interval>   Print stats every <interval> Bytes written to the target\n");
	fprintf(stderr, "STDIN: delta file. STDOUT: target file. STDERR: logging.\n");
}

static int stderr_logger (const char *fmt, ...) {
	va_list args;
	va_start (args, fmt);
	vfprintf (stderr, fmt, args);
	va_end (args);
	return 0;
}

int main(int argc, char* argv[]) {
	int opt;
	size_t log_interval = 0;
	vcdiff_log_t inst_log = NULL;

	while ((opt = getopt(argc, argv, "is:")) != -1) {
		switch (opt) {
			case 'i':
				inst_log = stderr_logger;
				break;
			case 's':
				log_interval = atoi(optarg) * 1024;
				break;
			default:
				usage();
				return 1;
		}
	}

	if (argc <= optind) {
		usage();
		return 1;
	}

	FILE *source = fopen(argv[optind], "r");
	if (source == NULL) {
		perror("Cannot open source_path");
		return 1;
	}

	int rc = apply_delta(stdin, source, stdout, log_interval, inst_log);

	fclose(source);

	return rc;
}