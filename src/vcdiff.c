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

#define READ_INT(VAR) \
	if (vcdiff_read_int(VAR, input, input_remainder) == VCDIFF_READ_CONT) return 0;

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
			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_INDICATOR);
			break;
		}
		default:
			RET_ERR(-1, "Reached invalid state");
	}

	return 0;
}

#define VCD_SOURCE 0x1
#define VCD_TARGET 0x2

static inline int _parse_win_hdr(vcdiff_t *ctx, const uint8_t **input, size_t *input_remainder) {
	switch (ctx->state) {
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_INDICATOR) {
			/* add the length of the preceding window */
			ctx->target_offset += ctx->win_window_len;

			/* reset state values */
			ctx->win_segment_len = 0;
			ctx->win_segment_pos = 0;
			ctx->win_window_len = 0;
			ctx->win_inst_len = 0;

			READ_BYTE(&ctx->win_indicator);
			if (ctx->win_indicator != VCD_SOURCE && ctx->win_indicator != VCD_TARGET && ctx->win_indicator != 0x00) {
				RET_ERR(-1, "Unsupported window indicator");
			}
			if (ctx->win_indicator == 0x00) {
				SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_DELTA_LEN);
				break; /* the parent method will bring us back */
			} else {
				SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_SEGMENT_LEN);
			}
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_SEGMENT_LEN) {
			READ_INT(&ctx->win_segment_len);
			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_SEGMENT_POS);
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_SEGMENT_POS) {
			READ_INT(&ctx->win_segment_pos);
			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_DELTA_LEN);
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_DELTA_LEN) {
			/* just skip the delta length ... we don't care. */
			uint32_t len = 0;
			READ_INT(&len);
			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_WINDOW_LENGTH);
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_WINDOW_LENGTH) {
			READ_INT(&ctx->win_window_len);
			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_DELTA_INDICATOR);
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_DELTA_INDICATOR) {
			uint8_t ind;
			READ_BYTE(&ind);
			if (ind != 0x00) RET_ERR(-1, "Unsupported delta indicator");
			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_DATA_LEN);
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_DATA_LEN) {
			uint8_t len;
			/* Of course, the VCDIFF standard defines this to be an
			 * integer. But with interleaved data and addresses, both
			 * of them must be zero. And integer 0 is also byte 0x00 */
			READ_BYTE(&len);
			if (len != 0x00) RET_ERR(-1, "Data length must be zero");
			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_INST_LEN);
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_INST_LEN) {
			READ_INT(&ctx->win_inst_len);
			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_ADDR_LEN);
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_ADDR_LEN) {
			uint8_t len;
			READ_BYTE(&len);
			if (len != 0x00) RET_ERR(-1, "Address length must be zero");
			SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_INST);
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
			case STATE_WIN_HDR:
				rc = _parse_win_hdr(ctx, &input, &input_remainder);
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
	ctx->target_offset = 0;
	ctx->win_window_len = 0;
}

int vcdiff_finish (vcdiff_t *ctx) {
	int rc = 0;

	if (ctx->state == STATE_FINISH) {
		rc = -1;
		ctx->error_msg = "Operation already finished";
	} else if (ctx->state != STATE_WIN_HDR + STATE_WIN_HDR_INDICATOR) {
		rc = -1;
		ctx->error_msg = "Unfinished vcdiff operation";
	}

	ctx->state = STATE_FINISH;

	return rc;
}
