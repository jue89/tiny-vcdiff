#include "vcdiff.h"
#include "vcdiff/state.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

static void test_vcdiff_header (void **state) {
	uint8_t data[] = {0xd6, 0xc3, 0xc4, 0x53, 0x00};
	vcdiff_t ctx;

	/* read header in one go */
	vcdiff_init(&ctx);
	assert_int_equal(ctx.state, 0);
	assert_ptr_equal(ctx.error_msg, NULL);
	assert_int_equal(vcdiff_apply_delta(&ctx, data, sizeof(data)), 0);
	assert_string_equal("STATE_WIN_INDICATOR", vcdiff_state_str(&ctx));
	assert_string_equal("NO ERROR", vcdiff_error_str(&ctx));

	/* finish operation */
	assert_int_equal(vcdiff_finish(&ctx), 0);
	assert_int_equal(vcdiff_finish(&ctx), -1);
	assert_string_equal("Operation already finished", vcdiff_error_str(&ctx));

	/* read header byte-wise */
	vcdiff_init(&ctx);
	for (size_t i = 0; i < sizeof(data); i++) {
		assert_int_equal(vcdiff_apply_delta(&ctx, &data[i], 1), 0);
	}
	assert_string_equal("STATE_WIN_INDICATOR", vcdiff_state_str(&ctx));
	assert_string_equal("NO ERROR", vcdiff_error_str(&ctx));

	/* fail with wrong header indicator */
	data[4] = 0x01;
	vcdiff_init(&ctx);
	assert_int_equal(vcdiff_apply_delta(&ctx, data, sizeof(data)), -1);
	assert_string_equal("STATE_HDR_INDICATOR", vcdiff_state_str(&ctx));
	assert_string_equal("Header indicator references unsupported features", vcdiff_error_str(&ctx));

	/* fail with wrong magic */
	data[3] = 0x00;
	vcdiff_init(&ctx);
	assert_int_equal(vcdiff_apply_delta(&ctx, data, sizeof(data)), -1);
	assert_string_equal("STATE_HDR_MAGIC3", vcdiff_state_str(&ctx));
	assert_string_equal("Invalid magic", vcdiff_error_str(&ctx));
}

int main (void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_vcdiff_header),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
