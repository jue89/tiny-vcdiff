#ifndef VCDIFF_H
#define VCDIFF_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
	const char *error_msg;
	uint16_t state;

	uint8_t win_indicator;
	uint32_t target_offset;
	uint32_t win_segment_len;
	uint32_t win_segment_pos;
	uint32_t win_window_len;
	uint32_t win_inst_len;
} vcdiff_t;

void vcdiff_init (vcdiff_t *ctx);

int vcdiff_apply_delta (vcdiff_t *ctx, const uint8_t *input, size_t input_remainder);

int vcdiff_finish (vcdiff_t *ctx);

#endif
