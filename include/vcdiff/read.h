#ifndef VCDIFF_READ_H
#define VCDIFF_READ_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
	VCDIFF_READ_DONE,
	VCDIFF_READ_CONT
} vcdiff_read_rc_t;

vcdiff_read_rc_t vcdiff_read_byte (uint8_t *dst, const uint8_t **input, size_t *input_remainder);

vcdiff_read_rc_t vcdiff_read_int (uint32_t *dst, const uint8_t **input, size_t *input_remainder);

vcdiff_read_rc_t vcdiff_read_buffer (uint8_t *buf, size_t *buf_ptr, size_t buf_len, const uint8_t **input, size_t *input_remainder);

#endif
