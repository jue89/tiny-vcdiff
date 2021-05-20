#include "vcdiff.h"
#include "vcdiff/read.h"
#include "vcdiff/state.h"
#include "assert.h"

#define STATE(MAJ, MIN) \
	case (MAJ + MIN): \
		assert(ctx->error_msg == NULL); \
		assert(ctx->state == MAJ + MIN);

#define SET_STATE(MAJ, MIN) \
	ctx->state = MAJ + MIN;

#define RET_ERR(RC, MSG) { \
	assert(RC != 0); \
	ctx->error_msg = MSG; \
	return RC; }

#define READ_BYTE(VAR) \
	if (vcdiff_read_byte(VAR, input, input_remainder) == VCDIFF_READ_CONT) return 0;

static const char *msg_invalid_magic = "Invalid magic";

static inline int _parse_hdr(vcdiff_t *ctx, const uint8_t **input, size_t *input_remainder) {
	switch (ctx->state) {
		STATE(STATE_HDR, STATE_HDR_MAGIC0) {
			uint8_t magic;
			READ_BYTE(&magic);
			if (magic != 0xd6) RET_ERR(-1, msg_invalid_magic);
			SET_STATE(STATE_HDR, STATE_HDR_MAGIC1);
		}
		STATE(STATE_HDR, STATE_HDR_MAGIC1) {
			uint8_t magic;
			READ_BYTE(&magic);
			if (magic != 0xc3) RET_ERR(-1, msg_invalid_magic);
			SET_STATE(STATE_HDR, STATE_HDR_MAGIC2);
		}
		STATE(STATE_HDR, STATE_HDR_MAGIC2) {
			uint8_t magic;
			READ_BYTE(&magic);
			if (magic != 0xc4) RET_ERR(-1, msg_invalid_magic);
			SET_STATE(STATE_HDR, STATE_HDR_MAGIC3);
		}
		STATE(STATE_HDR, STATE_HDR_MAGIC3) {
			uint8_t magic;
			READ_BYTE(&magic);
			if (magic != 0x53) RET_ERR(-1, msg_invalid_magic);
			SET_STATE(STATE_HDR, STATE_HDR_INDICATOR);
		}
		STATE(STATE_HDR, STATE_HDR_INDICATOR) {
			uint8_t ind;
			READ_BYTE(&ind);
			if (ind != 0x00) RET_ERR(-1, "Header indicator references unsupported features");
			SET_STATE(STATE_WIN, STATE_WIN_INDICATOR);
			break;
		}
		default:
			RET_ERR(-1, "Reached invalid state");
	}

	return 0;
}

int vcdiff_apply_delta (vcdiff_t *ctx, const uint8_t *input, size_t input_remainder) {
	int rc = 0;

	while (rc == 0 && input_remainder > 0) {
		switch (ctx->state & 0xff00) {
			case STATE_HDR:
				rc = _parse_hdr(ctx, &input, &input_remainder);
				break;
			default:
				RET_ERR(-1, "Reached invalid state");
		}
	}

	return rc;
}

void vcdiff_init (vcdiff_t *ctx) {
	SET_STATE(STATE_HDR, STATE_HDR_MAGIC0);
	ctx->error_msg = NULL;
}

int vcdiff_finish (vcdiff_t *ctx) {
	int rc = 0;

	if (ctx->state == STATE_FINISH) {
		rc = -1;
		ctx->error_msg = "Operation already finished";
	} else if (ctx->state != STATE_WIN + STATE_WIN_INDICATOR) {
		rc = -1;
		ctx->error_msg = "Unfinished vcdiff operation";
	}

	ctx->state = STATE_FINISH;

	return rc;
}
