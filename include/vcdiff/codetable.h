#ifndef VCDIFF_CODETABLE_H
#define VCDIFF_CODETABLE_H

#include <stddef.h>
#include <stdint.h>
#include "vcdiff.h"

enum {
	VCDIFF_INST_NOP,
	VCDIFF_INST_ADD,
	VCDIFF_INST_RUN,
	VCDIFF_INST_COPY
};

void vcdiff_codetable_decode(vcdiff_t *ctx, uint8_t instrcution);

#endif
