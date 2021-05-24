#include "vcdiff/codetable.h"

void vcdiff_codetable_decode(vcdiff_t *ctx, uint8_t code) {
	ctx->inst0 = VCDIFF_INST_NOP;
	ctx->inst1 = VCDIFF_INST_NOP;
	ctx->size0 = 0;
	ctx->size1 = 0;
	ctx->mode0 = 0;
	ctx->mode1 = 0;

	if (code == 0) {
		ctx->inst0 = VCDIFF_INST_RUN;
	} else if (code <= 18) {
		ctx->inst0 = VCDIFF_INST_ADD;
		if (code > 1) {
			ctx->size0 = code - 1;
		}
	} else if (code <= 162) {
		uint8_t col = (code - 19) % 16;
		uint8_t row = (code - 19) / 16;
		ctx->inst0 = VCDIFF_INST_COPY;
		if (col != 0) {
			ctx->size0 = col + 3;
		}
		ctx->mode0 = row;
	} else if (code <= 234) {
		uint8_t col = (code - 163) % 3;
		uint8_t row = (code - 163) / 3;
		ctx->inst0 = VCDIFF_INST_ADD;
		ctx->inst1 = VCDIFF_INST_COPY;
		ctx->size0 = (row % 4) + 1;
		ctx->size1 = (col % 3) + 4;
		ctx->mode1 = row / 4;
	} else if (code <= 246) {
		uint8_t col = (code - 235) % 4;
		uint8_t row = (code - 235) / 4;
		ctx->inst0 = VCDIFF_INST_ADD;
		ctx->inst1 = VCDIFF_INST_COPY;
		ctx->size0 = col + 1;
		ctx->size1 = 4;
		ctx->mode1 = row + 6;
	} else {
		uint8_t col = code - 247;
		ctx->inst0 = VCDIFF_INST_COPY;
		ctx->inst1 = VCDIFF_INST_ADD;
		ctx->size0 = 4;
		ctx->size1 = 1;
		ctx->mode0 = col;
	}
}