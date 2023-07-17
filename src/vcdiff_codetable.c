#include "vcdiff/codetable.h"

void vcdiff_codetable_decode(uint8_t * inst0, size_t * size0, uint8_t * mode0,
                             uint8_t * inst1, size_t * size1, uint8_t * mode1,
                             uint8_t code) {
	*inst0 = VCDIFF_INST_NOP;
	*inst1 = VCDIFF_INST_NOP;
	*size0 = 0;
	*size1 = 0;
	*mode0 = 0;
	*mode1 = 0;

	if (code == 0) {
		*inst0 = VCDIFF_INST_RUN;
	} else if (code <= 18) {
		*inst0 = VCDIFF_INST_ADD;
		if (code > 1) {
			*size0 = code - 1;
		}
	} else if (code <= 162) {
		uint8_t col = (code - 19) % 16;
		uint8_t row = (code - 19) / 16;
		*inst0 = VCDIFF_INST_COPY;
		if (col != 0) {
			*size0 = col + 3;
		}
		*mode0 = row;
	} else if (code <= 234) {
		uint8_t col = (code - 163) % 3;
		uint8_t row = (code - 163) / 3;
		*inst0 = VCDIFF_INST_ADD;
		*inst1 = VCDIFF_INST_COPY;
		*size0 = (row % 4) + 1;
		*size1 = (col % 3) + 4;
		*mode1 = row / 4;
	} else if (code <= 246) {
		uint8_t col = (code - 235) % 4;
		uint8_t row = (code - 235) / 4;
		*inst0 = VCDIFF_INST_ADD;
		*inst1 = VCDIFF_INST_COPY;
		*size0 = col + 1;
		*size1 = 4;
		*mode1 = row + 6;
	} else {
		uint8_t col = code - 247;
		*inst0 = VCDIFF_INST_COPY;
		*inst1 = VCDIFF_INST_ADD;
		*size0 = 4;
		*size1 = 1;
		*mode0 = col;
	}
}
