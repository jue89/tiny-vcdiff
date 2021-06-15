#ifndef VCDIFF_CODETABLE_H
#define VCDIFF_CODETABLE_H

#include <stddef.h>
#include <stdint.h>

enum {
	VCDIFF_INST_NOP,
	VCDIFF_INST_ADD,
	VCDIFF_INST_RUN,
	VCDIFF_INST_COPY
};

void vcdiff_codetable_decode(uint8_t * inst0, size_t * size0, uint8_t * mode0,
                             uint8_t * inst1, size_t * size1, uint8_t * mode1,
                             uint8_t code);

#endif
