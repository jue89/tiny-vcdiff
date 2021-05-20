#include "vcdiff/read.h"
#include <string.h>

vcdiff_read_rc_t vcdiff_read_byte (uint8_t *dst, const uint8_t **input, size_t *input_remainder) {
	if (*input_remainder == 0) return VCDIFF_READ_CONT;

	*dst = *(*input)++;
	(*input_remainder)--;
	
	return VCDIFF_READ_DONE;
}

vcdiff_read_rc_t vcdiff_read_int (uint32_t *dst, const uint8_t **input, size_t *input_remainder) {
	vcdiff_read_rc_t rc = VCDIFF_READ_CONT;

	while (rc == VCDIFF_READ_CONT && *input_remainder > 0) {
		*dst = (*dst << 7) | (**input & 0x7f);

		rc = (**input & 0x80) ? VCDIFF_READ_CONT : VCDIFF_READ_DONE;
		*(*input)++;
		(*input_remainder)--;
	}

	return rc;
}

vcdiff_read_rc_t vcdiff_read_buffer (uint8_t *buf, size_t *buf_ptr, size_t buf_len, const uint8_t **input, size_t *input_remainder) {
	while (1) {
		if (*buf_ptr >= buf_len) return VCDIFF_READ_DONE;
		if (*input_remainder == 0) return VCDIFF_READ_CONT;

		size_t copy_len = buf_len - *buf_ptr;
		if (*input_remainder < copy_len) copy_len = *input_remainder;
		memcpy(buf + *buf_ptr, *input, copy_len);
		*input += copy_len;
		*input_remainder -= copy_len;
		*buf_ptr += copy_len;
	};
	
	return VCDIFF_READ_CONT;
}
