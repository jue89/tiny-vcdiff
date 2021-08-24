#include "vcdiff.h"
#include "vcdiff/read.h"
#include "vcdiff/state.h"
#include "vcdiff/addrcache.h"
#include "vcdiff/codetable.h"
#include "assert.h"
#include <string.h>
#include <sys/types.h>

#define LOG(FMT, ...) \
	if (ctx->inst_log) ctx->inst_log(FMT, __VA_ARGS__);
#define LOG_STATE() \
	if (ctx->state_log) ctx->state_log("Enter state: %s\n", vcdiff_state_str(ctx));

#define STATE(MAJ, MIN) \
	case (MAJ + MIN): \
		assert(ctx->error_msg == NULL); \
		assert(ctx->state == MAJ + MIN); \
		LOG_STATE();


#define SET_STATE(MAJ, MIN) \
	ctx->state = MAJ + MIN;

#define RET_ERR(RC, MSG) { \
	assert(RC != 0); \
	ctx->error_msg = MSG; \
	return RC; }

#define READ_BYTE(VAR) { \
	int rc = vcdiff_read_byte(VAR, input, input_remainder); \
	if (rc != 0) return rc; }

#define READ_INT(VAR) { \
	int rc = vcdiff_read_int(VAR, input, input_remainder); \
	if (rc != 0) return rc; }

#define MIN(a, b) (a > b) ? b : a;

#define FIT_TO_BUFFER(LEN) MIN(LEN, sizeof(ctx->buffer))

#define READ_BUFFER(LEN) {\
	int rc = vcdiff_read_buffer(ctx->buffer, &ctx->buffer_ptr, LEN, input, input_remainder); \
	if (rc != 0) return rc; \
	ctx->buffer_ptr = 0; }

#define CALL(FN, ...) { \
	int rc = FN(ctx, input, input_remainder, __VA_ARGS__); \
	if (rc != 0) return rc; }

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
			READ_BYTE(&ctx->win_indicator);

			/* reset header info values */
			ctx->win_segment_len = 0;
			ctx->win_segment_pos = 0;
			ctx->win_window_len = 0;

			if (ctx->win_indicator != VCD_SOURCE && ctx->win_indicator != VCD_TARGET && ctx->win_indicator != 0x00) {
				RET_ERR(-1, "Unsupported window indicator");
			}

			if (ctx->win_indicator == 0x00) {
				LOG("WIN%s", "");
				SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_DELTA_LEN);
				break; /* the parent method will bring us back */
			} else {
				LOG("WIN %s ", (ctx->win_indicator == VCD_SOURCE) ? "VCD_SOURCE" : "VCD_TARGET");
				SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_SEGMENT_LEN);
			}
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_SEGMENT_LEN) {
			READ_INT(&ctx->win_segment_len);
			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_SEGMENT_POS);
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_SEGMENT_POS) {
			READ_INT(&ctx->win_segment_pos);
			LOG("[0x%x+%d]", ctx->win_segment_pos, ctx->win_segment_len);
			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_DELTA_LEN);
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_DELTA_LEN) {
			/* just skip the delta length ... we don't care. */
			size_t len = 0;
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
			/* skip instruction length ... we don't care. */
			size_t len = 0;
			READ_INT(&len);
			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_ADDR_LEN);
		}
		STATE(STATE_WIN_HDR, STATE_WIN_HDR_ADDR_LEN) {
			uint8_t len;
			READ_BYTE(&len);
			if (len != 0x00) RET_ERR(-1, "Address length must be zero");

			/* prepare instruction decoding */
			ctx->win_window_pos = 0;
			vcdiff_addrcache_init(&ctx->cache);

			/* prepare target window */
			if (ctx->target_driver->erase) {
				int rc = ctx->target_driver->erase(ctx->target_dev, ctx->target_offset, ctx->win_window_len);
				if (rc < 0) RET_ERR(rc, "Target erase failed");
			}

			LOG(" => [0x%0x+%d]\n", ctx->target_offset, ctx->win_window_len);

			SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_INST);
			break;
		}
		default:
			RET_ERR(-1, "Reached invalid state");
	}

	return 0;
}

static int _parse_win_body_addr(vcdiff_t *ctx, const uint8_t **input, size_t *input_remainder, uint8_t mode, size_t *addr) {
	switch (vcdiff_addrcache_get_mode(mode)) {
		case VCDIFF_MODE_SELF:
			READ_INT(addr);
			*addr = vcdiff_addrcache_decode_self(&ctx->cache, *addr);
			break;
		case VCDIFF_MODE_HERE:
			READ_INT(addr);
			*addr = vcdiff_addrcache_decode_here(&ctx->cache, ctx->win_segment_len + ctx->win_window_pos, *addr);
			break;
		case VCDIFF_MODE_NEAR:
			READ_INT(addr);
			*addr = vcdiff_addrcache_decode_near(&ctx->cache, mode, *addr);
			break;
		case VCDIFF_MODE_SAME:
			READ_BYTE((uint8_t*) addr);
			*addr = vcdiff_addrcache_decode_same(&ctx->cache, mode, *addr);
			break;
		default:
			RET_ERR(-1, "Invalid mode");
	}
	return 0;
}

static int _parse_win_body_exec(vcdiff_t *ctx, const uint8_t **input, size_t *input_remainder, uint8_t inst, size_t *size, size_t *addr) {
	int rc;

	if (ctx->win_window_pos + *size > ctx->win_window_len) {
		RET_ERR(-1, "Size out of bounds");
	}

	if (inst == VCDIFF_INST_ADD) {
		while (*size > 0) {
			size_t to_write = FIT_TO_BUFFER(*size);
			READ_BUFFER(to_write);
			LOG("  ADD => [0x%x+%d]\n", ctx->target_offset + ctx->win_window_pos, to_write);
			rc = ctx->target_driver->write(ctx->target_dev, ctx->buffer, ctx->target_offset + ctx->win_window_pos, to_write);
			if (rc < 0) RET_ERR(rc, "INST_ADD: cannot write to target");
			ctx->win_window_pos += to_write;
			*size -= to_write;
		}
		return 0;
	}

	if (inst == VCDIFF_INST_RUN) {
		uint8_t byte;
		READ_BYTE(&byte);
		while (*size > 0) {
			size_t to_write = FIT_TO_BUFFER(*size);
			memset(ctx->buffer, byte, to_write);
			LOG("  RUN 0x%02x => [0x%x+%d]\n", byte, ctx->target_offset + ctx->win_window_pos, to_write);
			rc = ctx->target_driver->write(ctx->target_dev, ctx->buffer, ctx->target_offset + ctx->win_window_pos, to_write);
			if (rc < 0) RET_ERR(rc, "INST_RUN: cannot write to target");
			ctx->win_window_pos += to_write;
			*size -= to_write;
		}
		return 0;
	}

	if (inst == VCDIFF_INST_COPY) {
		while (*size > 0) {
			const vcdiff_driver_t *driver = (ctx->win_indicator & VCD_SOURCE) ? ctx->source_driver : ctx->target_driver;
			void *dev = (ctx->win_indicator & VCD_SOURCE) ? ctx->source_dev : ctx->target_dev;

			size_t to_copy = FIT_TO_BUFFER(*size);

			/* read */
			if (*addr < ctx->win_segment_len) {
				/* data lives in the given segment */
				if (*addr + *size > ctx->win_segment_len) {
					RET_ERR(-1, "Address must not cross source boundary");
				}
				LOG("  COPY from SEGMENT [0x%x+%d]", *addr, to_copy);
				rc = driver->read(dev, ctx->buffer, ctx->win_segment_pos + *addr, to_copy);
			} else {
				/* data lives in the current window */
				ssize_t bytes_ahead = ctx->win_window_pos - (*addr - ctx->win_segment_len);
				if (bytes_ahead <= 0) {
					RET_ERR(-1, "Address is outside of available target window");
				}
				to_copy = MIN(to_copy, (size_t) bytes_ahead);
				LOG("  COPY from WINDOW [0x%x+%d]", *addr - ctx->win_segment_len, to_copy);
				rc = ctx->target_driver->read(ctx->target_dev, ctx->buffer, *addr - ctx->win_segment_len + ctx->target_offset, to_copy);
			}
			if (rc < 0) RET_ERR(rc, "INST_COPY: cannot read from target/source");

			/* write */
			LOG(" => [0x%x+%d]\n", ctx->target_offset + ctx->win_window_pos, to_copy);
			rc = ctx->target_driver->write(ctx->target_dev, ctx->buffer, ctx->target_offset + ctx->win_window_pos, to_copy);
			if (rc < 0) RET_ERR(rc, "INST_COPY: cannot write to target");

			ctx->win_window_pos += to_copy;
			*size -= to_copy;
			*addr += to_copy;
		}
		return 0;
	}

	RET_ERR(-1, "Invalid Instruction");
}

static inline int _parse_win_body(vcdiff_t *ctx, const uint8_t **input, size_t *input_remainder) {
	switch (ctx->state) {
		STATE(STATE_WIN_BODY, STATE_WIN_BODY_INST) {
			uint8_t code;
			READ_BYTE(&code);

			/* reset address store and buffer */
			ctx->addr0 = 0;
			ctx->addr1 = 0;

			vcdiff_codetable_decode(&ctx->inst0, &ctx->size0, &ctx->mode0,
			                        &ctx->inst1, &ctx->size1, &ctx->mode1, code);
			if (ctx->size0 == 0) {
				SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_SIZE0);
			} else if (ctx->inst0 == VCDIFF_INST_COPY) {
				SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_ADDR0);
				break;
			} else {
				SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_EXEC0);
				break;
			}
		}
		STATE(STATE_WIN_BODY, STATE_WIN_BODY_SIZE0) {
			READ_INT(&ctx->size0);
			if (ctx->inst0 == VCDIFF_INST_COPY) {
				SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_ADDR0);
			} else {
				SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_EXEC0);
				break;
			}
		}
		STATE(STATE_WIN_BODY, STATE_WIN_BODY_ADDR0) {
			CALL(_parse_win_body_addr, ctx->mode0, &ctx->addr0);
			SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_EXEC0);
		}
		STATE(STATE_WIN_BODY, STATE_WIN_BODY_EXEC0) {
			CALL(_parse_win_body_exec, ctx->inst0, &ctx->size0, &ctx->addr0);
			if (ctx->win_window_pos >= ctx->win_window_len) {
				SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_STATE_WIN_BODY_FINISH);
				break;
			} else if (ctx->inst1 != VCDIFF_INST_NOP) {
				if (ctx->size0 == 0) {
					SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_SIZE1);
				} else if (ctx->inst1 == VCDIFF_INST_COPY) {
					SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_ADDR1);
					break;
				} else {
					SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_EXEC1);
					break;
				}
			} else {
				SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_INST);
				break;
			}
		}
		STATE(STATE_WIN_BODY, STATE_WIN_BODY_SIZE1) {
			READ_INT(&ctx->size1);
			if (ctx->inst1 == VCDIFF_INST_COPY) {
				SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_ADDR1);
			} else {
				SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_EXEC1);
				break;
			}
		}
		STATE(STATE_WIN_BODY, STATE_WIN_BODY_ADDR1) {
			CALL(_parse_win_body_addr, ctx->mode1, &ctx->addr1);
			SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_EXEC1);
		}
		STATE(STATE_WIN_BODY, STATE_WIN_BODY_EXEC1) {
			CALL(_parse_win_body_exec, ctx->inst1, &ctx->size1, &ctx->addr1);
			if (ctx->win_window_pos >= ctx->win_window_len) {
				SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_STATE_WIN_BODY_FINISH);
			} else {
				SET_STATE(STATE_WIN_BODY, STATE_WIN_BODY_INST);
				break;
			}
		}
		STATE(STATE_WIN_BODY, STATE_WIN_BODY_STATE_WIN_BODY_FINISH) {
			/* flush data */
			if (ctx->target_driver->flush) {
				int rc = ctx->target_driver->flush(ctx->target_dev, ctx->target_offset, ctx->win_window_len);
				if (rc < 0) RET_ERR(rc, "Target flush failed");
			}

			/* add the length of the processed window */
			ctx->target_offset += ctx->win_window_len;

			SET_STATE(STATE_WIN_HDR, STATE_WIN_HDR_INDICATOR);
			break;
		}

		default:
			RET_ERR(-1, "Reached invalid state");
	}

	return 0;
}

int vcdiff_apply_delta (vcdiff_t *ctx, const uint8_t *input, size_t input_remainder) {
	int rc = 0;

	/* make sure drivers are attached */
	assert(ctx->target_driver && ctx->target_driver->read && ctx->target_driver->write);
	assert(ctx->source_driver && ctx->source_driver->read);

	while (rc == 0) {
		switch (ctx->state & 0xff00) {
			case STATE_HDR:
				rc = _parse_hdr(ctx, &input, &input_remainder);
				break;
			case STATE_WIN_HDR:
				rc = _parse_win_hdr(ctx, &input, &input_remainder);
				break;
			case STATE_WIN_BODY:
				rc = _parse_win_body(ctx, &input, &input_remainder);
				break;
			default:
				RET_ERR(-1, "Reached invalid state");
		}
	}

	/* mask out continue return codes */
	if (rc > 0) rc = 0;

	return rc;
}

void vcdiff_init (vcdiff_t *ctx) {
	SET_STATE(STATE_HDR, STATE_HDR_MAGIC0);
	ctx->error_msg = NULL;
	ctx->target_offset = 0;
	ctx->buffer_ptr = 0;
	ctx->target_driver = NULL;
	ctx->source_driver = NULL;
	ctx->inst_log = NULL;
	ctx->state_log = NULL;
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
