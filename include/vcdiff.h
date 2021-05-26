#ifndef VCDIFF_H
#define VCDIFF_H

#include "vcdiff/addrcache.h"

#include <stdint.h>
#include <stddef.h>

#ifndef VCDIFF_BUFFER_SIZE
#define VCDIFF_BUFFER_SIZE (1024 * 1024)
#endif

typedef int (*vcdiff_driver_read_t)(void *dev, uint8_t *dest, size_t offset, size_t len);
typedef int (*vcdiff_driver_write_t)(void *dev, uint8_t *src, size_t offset, size_t len);
typedef int (*vcdiff_driver_flush_t)(void *dev, size_t offset, size_t len);
typedef int (*vcdiff_driver_erase_t)(void *dev, size_t offset, size_t len);
typedef int (*vcdiff_log_t)(const char *fmt, ...);

typedef struct {
	vcdiff_driver_read_t read;
	vcdiff_driver_write_t write;
	vcdiff_driver_flush_t flush;
	vcdiff_driver_erase_t erase;
} vcdiff_driver_t;

typedef struct {
	vcdiff_log_t inst_log;
	vcdiff_log_t state_log;

	const vcdiff_driver_t *source_driver;
	void *source_dev;
	const vcdiff_driver_t *target_driver;
	void *target_dev;

	const char *error_msg;
	uint16_t state;

	uint8_t win_indicator;
	size_t target_offset;
	size_t win_segment_len;
	size_t win_segment_pos;
	size_t win_window_len;
	size_t win_window_pos;

	vcdiff_cache_t cache;
	uint8_t buffer[VCDIFF_BUFFER_SIZE];
	size_t buffer_ptr;

	uint8_t inst0;
	uint8_t inst1;
	uint8_t mode0;
	uint8_t mode1;
	size_t size0;
	size_t size1;
	size_t addr0;
	size_t addr1;
} vcdiff_t;

void vcdiff_init (vcdiff_t *ctx);

static inline void vcdiff_set_target_driver (vcdiff_t *ctx, const vcdiff_driver_t *driver, void *dev) {
	ctx->target_driver = driver;
	ctx->target_dev = dev;
}

static inline void vcdiff_set_source_driver (vcdiff_t *ctx, const vcdiff_driver_t *driver, void *dev) {
	ctx->source_driver = driver;
	ctx->source_dev = dev;
}

static inline void vcdiff_set_logger (vcdiff_t *ctx, vcdiff_log_t inst_log, vcdiff_log_t state_log) {
	ctx->inst_log = inst_log;
	ctx->state_log = state_log;
}

int vcdiff_apply_delta (vcdiff_t *ctx, const uint8_t *input, size_t len);

int vcdiff_finish (vcdiff_t *ctx);

#endif
