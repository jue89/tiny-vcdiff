#ifndef VCDIFF_H
#define VCDIFF_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint16_t state;
	const char *error_msg;
} vcdiff_t;

void vcdiff_init (vcdiff_t *ctx);

int vcdiff_apply_delta (vcdiff_t *ctx, const uint8_t *input, size_t input_remainder);

int vcdiff_finish (vcdiff_t *ctx);

#endif
