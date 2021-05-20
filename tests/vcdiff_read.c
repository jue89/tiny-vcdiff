#include "vcdiff/read.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

static void test_vcdiff_read_byte (void **state) {
	const uint8_t data[] = {0x42};
	const uint8_t *ptr = data;
	size_t remaining_bytes = 1;
	uint8_t byte;

	/* read first byte */
	assert_int_equal(vcdiff_read_byte(&byte, &ptr, &remaining_bytes), VCDIFF_READ_DONE);
	assert_int_equal(byte, data[0]);
	assert_int_equal(remaining_bytes, 0);

	/* read from empty buffer */
	assert_int_equal(vcdiff_read_byte(&byte, &ptr, &remaining_bytes), VCDIFF_READ_CONT);
}

static void test_vcdiff_read_int (void **state) {
	const uint32_t res_expected = 123456789;
	const uint8_t data[] = {0x80 + 58, 0x80 + 111, 0x80 + 26, 21};
	const uint8_t *ptr = data;
	size_t remaining_bytes = 3;
	uint32_t res = 0;

	/* read first three bytes */
	assert_int_equal(vcdiff_read_int(&res, &ptr, &remaining_bytes), VCDIFF_READ_CONT);
	assert_int_equal(remaining_bytes, 0);

	/* read last byte */
	remaining_bytes = 1;
	assert_int_equal(vcdiff_read_int(&res, &ptr, &remaining_bytes), VCDIFF_READ_DONE);
	assert_int_equal(remaining_bytes, 0);
	assert_int_equal(res, res_expected);

	/* read from empty buffer */
	assert_int_equal(vcdiff_read_int(&res, &ptr, &remaining_bytes), VCDIFF_READ_CONT);
}

static void test_vcdiff_read_buffer (void **state) {
	const uint8_t data[] = "0123456789";
	const uint8_t *ptr;
	uint8_t buf[4];
	size_t buf_idx;
	size_t remaining_bytes;

	/* read buffer in one go */
	ptr = data;
	remaining_bytes = strlen(data);
	buf_idx = 0;
	assert_int_equal(vcdiff_read_buffer(buf, &buf_idx, sizeof(buf), &ptr, &remaining_bytes), VCDIFF_READ_DONE);
	assert_memory_equal(data, buf, sizeof(buf));
	assert_int_equal(buf_idx, sizeof(buf));
	assert_int_equal(remaining_bytes, strlen(data) - sizeof(buf));

	/* read buffer in multiple steps */
	ptr = data;
	remaining_bytes = 3;
	buf_idx = 0;
	assert_int_equal(vcdiff_read_buffer(buf, &buf_idx, sizeof(buf), &ptr, &remaining_bytes), VCDIFF_READ_CONT);
	remaining_bytes = 1;
	assert_int_equal(vcdiff_read_buffer(buf, &buf_idx, sizeof(buf), &ptr, &remaining_bytes), VCDIFF_READ_DONE);
	assert_memory_equal(data, buf, sizeof(buf));

	/* read from empty buffer */
	remaining_bytes = 0;
	buf_idx = 0;
	assert_int_equal(vcdiff_read_buffer(buf, &buf_idx, sizeof(buf), &ptr, &remaining_bytes), VCDIFF_READ_CONT);
}

int main (void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_vcdiff_read_byte),
		cmocka_unit_test(test_vcdiff_read_int),
		cmocka_unit_test(test_vcdiff_read_buffer),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
