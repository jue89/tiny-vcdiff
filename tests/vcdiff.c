#include "vcdiff.h"
#include "vcdiff/state.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <stdio.h>

int target_erase (void *dev, size_t offset, size_t len) {
	check_expected_ptr(dev);
	check_expected(offset);
	check_expected(len);
	return (int) mock();
}

#define expect_target_erase(RC, DEV, OFFSET, LEN) \
	expect_value(target_erase, dev, DEV); \
	expect_value(target_erase, offset, OFFSET); \
	expect_value(target_erase, len, LEN); \
	will_return(target_erase, RC);

int target_write (void *dev, uint8_t *src, size_t offset, size_t len) {
	check_expected_ptr(dev);
	check_expected_ptr(src);
	check_expected(offset);
	check_expected(len);
	return (int) mock();
}

#define expect_target_write(RC, DEV, SRC, OFFSET, LEN) \
	expect_value(target_write, dev, DEV); \
	expect_value(target_write, src, SRC); \
	expect_value(target_write, offset, OFFSET); \
	expect_value(target_write, len, LEN); \
	will_return(target_write, RC);

int target_read (void *dev, uint8_t *src, size_t offset, size_t len) {
	check_expected_ptr(dev);
	check_expected_ptr(src);
	check_expected(offset);
	check_expected(len);
	return (int) mock();
}

#define expect_target_read(RC, DEV, SRC, OFFSET, LEN) \
	expect_value(target_read, dev, DEV); \
	expect_value(target_read, src, SRC); \
	expect_value(target_read, offset, OFFSET); \
	expect_value(target_read, len, LEN); \
	will_return(target_read, RC);

int target_flush (void *dev, size_t offset, size_t len) {
	check_expected_ptr(dev);
	check_expected(offset);
	check_expected(len);
	return (int) mock();
}

#define expect_target_flush(RC, DEV, OFFSET, LEN) \
	expect_value(target_flush, dev, DEV); \
	expect_value(target_flush, offset, OFFSET); \
	expect_value(target_flush, len, LEN); \
	will_return(target_flush, RC);

static vcdiff_driver_t target_driver = {
	.erase = target_erase,
	.write = target_write,
	.read = target_read,
	.flush = target_flush
};

int source_read (void *dev, uint8_t *dst, size_t offset, size_t len) {
	check_expected_ptr(dev);
	check_expected_ptr(dst);
	check_expected(offset);
	check_expected(len);
	return (int) mock();
}

#define expect_source_read(RC, DEV, DST, OFFSET, LEN) \
	expect_value(source_read, dev, DEV); \
	expect_value(source_read, dst, DST); \
	expect_value(source_read, offset, OFFSET); \
	expect_value(source_read, len, LEN); \
	will_return(source_read, RC);

static vcdiff_driver_t source_driver = {
	.read = source_read
};

static void test_vcdiff_header (void **state) {
	(void) state;
	uint8_t data[] = {0xd6, 0xc3, 0xc4, 0x53, 0x00};
	vcdiff_t ctx;

	/* read header in one go */
	vcdiff_init(&ctx);
	assert_int_equal(ctx.state, 0);
	assert_ptr_equal(ctx.error_msg, NULL);
	assert_int_equal(vcdiff_apply_delta(&ctx, data, sizeof(data)), 0);
	assert_string_equal("STATE_WIN_HDR_INDICATOR", vcdiff_state_str(&ctx));
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
	assert_string_equal("STATE_WIN_HDR_INDICATOR", vcdiff_state_str(&ctx));
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

static void test_vcdiff_win_header (void **state) {
	(void) state;
	uint8_t data[] = {0xD6, 0xC3, 0xC4, 0x53, 0x00, 0x00, 0x1D, 0x81, 0x89, 0x28, 0x00, 0x00, 0x16, 0x00};
	vcdiff_t ctx = {.target_driver = &target_driver, .target_dev = (void*) 0x42};

	/* read in one go */
	vcdiff_init(&ctx);
	expect_target_erase(0, 0x42, 0, 0x44a8);
	assert_int_equal(vcdiff_apply_delta(&ctx, data, sizeof(data)), 0);
	assert_string_equal("NO ERROR", vcdiff_error_str(&ctx));
	assert_string_equal("STATE_WIN_BODY_INST", vcdiff_state_str(&ctx));
	assert_int_equal(ctx.win_indicator, 0x00);
	assert_int_equal(ctx.target_offset, 0x00);
	assert_int_equal(ctx.win_segment_len, 0x00);
	assert_int_equal(ctx.win_segment_pos, 0x00);
	assert_int_equal(ctx.win_window_len, 0x44a8);

	/* read byte-wise */
	vcdiff_init(&ctx);
	expect_target_erase(0, 0x42, 0, 0x44a8);
	for (size_t i = 0; i < sizeof(data); i++) {
		assert_int_equal(vcdiff_apply_delta(&ctx, &data[i], 1), 0);
	}
	assert_string_equal("NO ERROR", vcdiff_error_str(&ctx));
	assert_string_equal("STATE_WIN_BODY_INST", vcdiff_state_str(&ctx));
	assert_int_equal(ctx.win_window_len, 0x44a8);
}

static void test_vcdiff_win_header_seg (void **state) {
	(void) state;
	uint8_t data[] = {0xD6, 0xC3, 0xC4, 0x53, 0x00, 0x01, 0x81, 0x89, 0x28, 0x00, 0x1D, 0x81, 0x89, 0x28, 0x00, 0x00, 0x16, 0x00};
	vcdiff_t ctx = {.target_driver = &target_driver, .target_dev = (void*) 0x42};

	/* read in one go */
	vcdiff_init(&ctx);
	expect_target_erase(0, 0x42, 0, 0x44a8);
	assert_int_equal(vcdiff_apply_delta(&ctx, data, sizeof(data)), 0);
	assert_string_equal("NO ERROR", vcdiff_error_str(&ctx));
	assert_string_equal("STATE_WIN_BODY_INST", vcdiff_state_str(&ctx));
	assert_int_equal(ctx.win_indicator, 0x01);
	assert_int_equal(ctx.target_offset, 0x00);
	assert_int_equal(ctx.win_segment_len, 0x44a8);
	assert_int_equal(ctx.win_segment_pos, 0x00);
	assert_int_equal(ctx.win_window_len, 0x44a8);

	/* read byte-wise */
	vcdiff_init(&ctx);
	expect_target_erase(0, 0x42, 0, 0x44a8);
	for (size_t i = 0; i < sizeof(data); i++) {
		assert_int_equal(vcdiff_apply_delta(&ctx, &data[i], 1), 0);
	}
	assert_string_equal("NO ERROR", vcdiff_error_str(&ctx));
	assert_string_equal("STATE_WIN_BODY_INST", vcdiff_state_str(&ctx));
	assert_int_equal(ctx.win_window_len, 0x44a8);
}

static void test_vcdiff_win_body1 (void **state) {
	(void) state;
	uint8_t data[] = {0xD6, 0xC3, 0xC4, 0x53, 0x00, 0x01, 0x81, 0x89, 0x28, 0x00, 0x1D, 0x81, 0x89, 0x28, 0x00, 0x00, 0x16, 0x00, 0x11, 0x52, 0x49, 0x4F, 0x54, 0xDF, 0x1A, 0x99, 0x60, 0x00, 0x14, 0x00, 0x08, 0x1A, 0x35, 0xC3, 0x1A, 0x13, 0x81, 0x89, 0x18, 0x10};
	vcdiff_t ctx = {
		.target_driver = &target_driver,
		.target_dev = (void*) 0x42,
		.source_driver = &source_driver,
		.source_dev = (void*) 0x43,
		//.debug_log = printf
	};

	/* read in one go */
	vcdiff_init(&ctx);
	expect_target_erase(0, 0x42, 0, 0x44a8);
	uint32_t len = 0x10;
	uint32_t offset = 0x0;
	while (len) {
		uint32_t chunk = len > VCDIFF_BUFFER_SIZE ? VCDIFF_BUFFER_SIZE : len;
		expect_target_write(0, 0x42, ctx.buffer, offset, chunk);
		len -= chunk;
		offset += chunk;
	}
	len = 0x4498;
	offset = 0x10;
	while (len) {
		uint32_t chunk = len > VCDIFF_BUFFER_SIZE ? VCDIFF_BUFFER_SIZE : len;
		expect_source_read(0, 0x43, ctx.buffer, offset, chunk);
		expect_target_write(0, 0x42, ctx.buffer, offset, chunk);
		len -= chunk;
		offset += chunk;
	}
	expect_target_flush(0, 0x42, 0, 0x44a8);
	assert_int_equal(vcdiff_apply_delta(&ctx, data, sizeof(data)), 0);
	assert_int_equal(vcdiff_finish(&ctx), 0);

	/* read byte-wise */
	vcdiff_init(&ctx);
	ctx.debug_log = NULL;
	expect_target_erase(0, 0x42, 0, 0x44a8);
	len = 0x10;
	offset = 0x0;
	while (len) {
		uint32_t chunk = len > VCDIFF_BUFFER_SIZE ? VCDIFF_BUFFER_SIZE : len;
		expect_target_write(0, 0x42, ctx.buffer, offset, chunk);
		len -= chunk;
		offset += chunk;
	}
	len = 0x4498;
	offset = 0x10;
	while (len) {
		uint32_t chunk = len > VCDIFF_BUFFER_SIZE ? VCDIFF_BUFFER_SIZE : len;
		expect_source_read(0, 0x43, ctx.buffer, offset, chunk);
		expect_target_write(0, 0x42, ctx.buffer, offset, chunk);
		len -= chunk;
		offset += chunk;
	}
	expect_target_flush(0, 0x42, 0, 0x44a8);
	for (size_t i = 0; i < sizeof(data); i++) {
		assert_int_equal(vcdiff_apply_delta(&ctx, &data[i], 1), 0);
	}
	assert_int_equal(vcdiff_finish(&ctx), 0);
}

static void test_vcdiff_win_body2 (void **state) {
	(void) state;
	uint8_t data[] = {0xD6, 0xC3, 0xC4, 0x53, 0x00, 0x01, 0x00, 0x00, 0x16, 0x82, 0x44, 0x00, 0x00, 0x10, 0x00, 0x02, 0x61, 0x73, 0x82, 0x17, 0x00, 0x05, 0x0A, 0x31, 0x32, 0x33, 0x23, 0x27, 0x03, 0x02, 0x0A};
	vcdiff_t ctx = {
		.target_driver = &target_driver,
		.target_dev = (void*) 0x42,
		.source_driver = &source_driver,
		.source_dev = (void*) 0x43,
		//.debug_log = printf
	};

	/* read in one go */
	vcdiff_init(&ctx);
	expect_target_erase(0, 0x42, 0, 0x144);
	expect_target_write(0, 0x42, ctx.buffer, 0x00, 0x1);
	uint32_t len = 279;
	uint32_t roffset = 0x0;
	uint32_t woffset = 0x1;
	while (len) {
		uint32_t chunk = 1;
		expect_target_read(0, 0x42, ctx.buffer, roffset, chunk);
		expect_target_write(0, 0x42, ctx.buffer, woffset, chunk);
		len -= chunk;
		roffset += chunk;
		woffset += chunk;
	}

	len = 0x4;
	woffset = 0x118;
	while (len) {
		uint32_t chunk = len > VCDIFF_BUFFER_SIZE ? VCDIFF_BUFFER_SIZE : len;
		expect_target_write(0, 0x42, ctx.buffer, woffset, chunk);
		len -= chunk;
		woffset += chunk;
	}

	len = 39;
	roffset = 0x119;
	woffset = 0x11c;
	while (len) {
		uint32_t chunk = 3 > VCDIFF_BUFFER_SIZE ? VCDIFF_BUFFER_SIZE : 3;
		expect_target_read(0, 0x42, ctx.buffer, roffset, chunk);
		expect_target_write(0, 0x42, ctx.buffer, woffset, chunk);
		len -= chunk;
		roffset += chunk;
		woffset += chunk;
	}
	expect_target_write(0, 0x42, ctx.buffer, 0x143, 1);

	expect_target_flush(0, 0x42, 0, 0x144);
	assert_int_equal(vcdiff_apply_delta(&ctx, data, sizeof(data)), 0);
	assert_int_equal(vcdiff_finish(&ctx), 0);
}

/* Missing tests:
- RUN
- 2nd INST
*/

int main (void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_vcdiff_header),
		cmocka_unit_test(test_vcdiff_win_header),
		cmocka_unit_test(test_vcdiff_win_header_seg),
		cmocka_unit_test(test_vcdiff_win_body1),
		cmocka_unit_test(test_vcdiff_win_body2),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
