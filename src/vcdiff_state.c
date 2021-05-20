#include "vcdiff/state.h"

static const char *state_hdr_str[] = {
	FOREACH_STATE_HDR(GENERATE_STRING)
};

static const char *state_win_hdr_str[] = {
	FOREACH_STATE_WIN_HDR(GENERATE_STRING)
};

static const char *state_win_body_str[] = {
	FOREACH_STATE_WIN_BODY(GENERATE_STRING)
};

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

const char *vcdiff_state_str (vcdiff_t *ctx) {
	uint16_t state_maj = (ctx->state & 0xff00);
	uint16_t state_min = (ctx->state & 0x00ff);
	switch (state_maj) {
		case STATE_HDR:
			if (state_min >= ARRAY_LENGTH(state_hdr_str)) break;
			return state_hdr_str[state_min];
		case STATE_WIN_HDR:
			if (state_min >= ARRAY_LENGTH(state_win_hdr_str)) break;
			return state_win_hdr_str[state_min];
		case STATE_WIN_BODY:
			if (state_min >= ARRAY_LENGTH(state_win_body_str)) break;
			return state_win_body_str[state_min];
	}
	return "UNKNOWN STATE!";
}

const char *vcdiff_error_str (vcdiff_t *ctx) {
	if (ctx->error_msg) return ctx->error_msg;
	else return "NO ERROR";
}
