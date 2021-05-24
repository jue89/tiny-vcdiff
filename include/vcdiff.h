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

typedef struct {
	vcdiff_driver_read_t read;
	vcdiff_driver_write_t write;
	vcdiff_driver_flush_t flush;
	vcdiff_driver_erase_t erase;
} vcdiff_driver_t;

typedef struct {
	vcdiff_driver_t *source_driver;
	void *source_dev;
	vcdiff_driver_t *target_driver;
	void *target_dev;

	const char *error_msg;
	uint16_t state;

	uint8_t win_indicator;
	uint32_t target_offset;
	uint32_t win_segment_len;
	uint32_t win_segment_pos;
	uint32_t win_window_len;
	uint32_t win_window_pos;

	vcdiff_cache_t cache;
	uint8_t buffer[VCDIFF_BUFFER_SIZE];
	size_t buffer_ptr;

	uint8_t inst0;
	uint8_t inst1;
	uint8_t mode0;
	uint8_t mode1;
	uint32_t size0;
	uint32_t size1;
	uint32_t addr0;
	uint32_t addr1;
} vcdiff_t;

void vcdiff_init (vcdiff_t *ctx);

int vcdiff_apply_delta (vcdiff_t *ctx, const uint8_t *input, size_t input_remainder);

int vcdiff_finish (vcdiff_t *ctx);

#endif
