/*
 * Copyright (C) 2021 Juergen Fitschen <me@jue.yt>
 *
 * This file is subject to the terms and conditions of the MIT License.
 * See the file LICENSE in the top level directory for more details.
 */

/**
 * @defgroup    Tiny VCDIFF Decoder
 * @brief       Decoder for interleaved VCDIFF deltas produced by open-vcdiff
 *
 * @{
 *
 * @file
 *
 * @author      Juergen Fitschen <me@jue.yt>
 */

#ifndef VCDIFF_H
#define VCDIFF_H

#include "vcdiff/addrcache.h"
#include "vcdiff/state.h"

#include <stdint.h>
#include <stddef.h>

#ifndef VCDIFF_BUFFER_SIZE
/**
 * @brief   Decoder buffer size in byte
 *
 * If an instruction requires more space, it is split into smaller chunks.
 * Using a buffer with 1 byte size is possible but causes a lot of IO operations.
 */
#define VCDIFF_BUFFER_SIZE (1024 * 1024)
#endif

/**
 * @brief   Signature for read operations
 *
 * @param      dev       Driver context
 * @param[out] dest      Pointer to the buffer to which the requested data should be written to
 * @param[in]  offset    Offset in byte on the device to start reading at
 * @param[in]  len       Amount of bytes to be read an written to dest
 * @return     `>= 0` if data has been read successfully
 * @return     `< 0` if an error occured while reading
 */
typedef int (*vcdiff_driver_read_t)(void *dev, uint8_t *dest, size_t offset, size_t len);

/**
 * @brief   Signature for write operations
 *
 * @param      dev       Driver context
 * @param[in]  src       Pointer to the buffer of data to write
 * @param[in]  offset    Offset in byte on the device to start writing to
 * @param[in]  len       Amount of bytes to be written to the device
 * @return     `>= 0` if data has been written successfully
 * @return     `< 0` if an error occured while writing
 */
typedef int (*vcdiff_driver_write_t)(void *dev, uint8_t *src, size_t offset, size_t len);

/**
 * @brief   Signature for flush operations
 *
 * @param      dev       Driver context
 * @param[in]  offset    Offset in byte on the device to flush
 * @param[in]  len       Amount of bytes to be flushed
 * @return     `>= 0` if data has been flushed successfully
 * @return     `< 0` if an error occured while flushing
 */
typedef int (*vcdiff_driver_flush_t)(void *dev, size_t offset, size_t len);

/**
 * @brief   Signature for erase operations
 *
 * @param      dev       Driver context
 * @param[in]  offset    Offset in byte on the device to erase
 * @param[in]  len       Amount of bytes to be erased
 * @return     `>= 0` if data has been erased successfully
 * @return     `< 0` if an error occured while erasing
 */
typedef int (*vcdiff_driver_erase_t)(void *dev, size_t offset, size_t len);

/**
 * @brief   Signature for logging callbacks
 *
 * `printf` from <stdio.h> can be used.
 */
typedef int (*vcdiff_log_t)(const char *fmt, ...);

/**
 * @brief   Driver defintion for source and target
 *
 * This driver is the glue between this library and the actual memory.
 */
typedef struct {
	vcdiff_driver_read_t read;   /**< Mandatory for source and target driver. */
	vcdiff_driver_write_t write; /**< Mandatory for target driver. */
	vcdiff_driver_flush_t flush; /**< Optional for target driver. Is called after a VCDIFF window has been written. */
	vcdiff_driver_erase_t erase; /**< Optional for target driver. Is called before a VCDIFF window is written. */
} vcdiff_driver_t;

/**
 * @brief   Decoder context
 *
 * The context holds the state of the decoder. Since the decoders allows the incoming
 * delta file to be split into chunks of arbitrary size, no state is hold on the stack.
 */
typedef struct {
#if !defined(VCDIFF_NDEBUG)
	const char *error_msg;                /**< Message of the last error */
	vcdiff_log_t inst_log;                /**< Callback for instruction logging */
	vcdiff_log_t state_log;               /**< Callback for state logging */
#endif

	const vcdiff_driver_t *source_driver; /**< Source driver defintion */
	void *source_dev;                     /**< Context for source driver */
	const vcdiff_driver_t *target_driver; /**< Target driver defintion */
	void *target_dev;                     /**< Context for target driver */

	uint16_t state;                       /**< Current decoder state */

	uint8_t win_indicator;
	size_t target_offset;
	size_t win_segment_len;
	size_t win_segment_pos;
	size_t win_window_len;
	size_t win_window_pos;

	vcdiff_cache_t cache;                /**< Context for the address cache */
	uint8_t buffer[VCDIFF_BUFFER_SIZE];  /**< Buffer for ADD, RUN and COPY instructions */
	size_t buffer_ptr;

	uint8_t inst0;
	uint8_t inst1;
	uint8_t mode0;
	uint8_t mode1;
	size_t size0;
	size_t size1;
	size_t addr0;
	size_t addr1;
} vcdiff_t;

/**
 * @brief   Intializes the decoder context
 *
 * @param      ctx       Decoder context
 */
void vcdiff_init (vcdiff_t *ctx);

/**
 * @brief   Connects decoder context and target device driver
 *
 * @param      ctx       Decoder context
 * @param[in]  driver    Driver definition for the target device
 * @param[in]  dev       Driver context for the target device
 */
static inline void vcdiff_set_target_driver (vcdiff_t *ctx, const vcdiff_driver_t *driver, void *dev) {
	ctx->target_driver = driver;
	ctx->target_dev = dev;
}

/**
 * @brief   Connects decoder context and source device driver
 *
 * @param      ctx       Decoder context
 * @param[in]  driver    Driver definition for the source device
 * @param[in]  dev       Driver context for the source device
 */
static inline void vcdiff_set_source_driver (vcdiff_t *ctx, const vcdiff_driver_t *driver, void *dev) {
	ctx->source_driver = driver;
	ctx->source_dev = dev;
}

/**
 * @brief   Connects decoder context and logging callbacks
 *
 * @param      ctx       Decoder context
 * @param[in]  inst_log  Instruction logging. Set to `NULL` to disable logging.
 * @param[in]  state_log State change logging. Set to `NULL` to disable logging.
 */
static inline void vcdiff_set_logger (vcdiff_t *ctx, vcdiff_log_t inst_log, vcdiff_log_t state_log) {
#if defined(VCDIFF_NDEBUG)
	(void) ctx;
	(void) inst_log;
	(void) state_log;
#else
	ctx->inst_log = inst_log;
	ctx->state_log = state_log;
#endif
}

/**
 * @brief   Applys the given delta file
 *
 * Deltas can be applied partially. Passing in the delta file byte by byte is supported.
 *
 * @param      ctx       Decoder context
 * @param[in]  input     Pointer to delta data
 * @param[in]  len       Length of provieded delta data
 * @return `0` if the provided delta data has been fully processed
 * @return `<0` if an error occured during processing
 */
int vcdiff_apply_delta (vcdiff_t *ctx, const uint8_t *input, size_t len);

/**
 * @brief   Finishes decoding
 *
 * @param      ctx       Decoder context
 * @return `0` if all provided delta data has been processed and no further data is awaited
 * @return `<0` if the operation is unfinished
 */
int vcdiff_finish (vcdiff_t *ctx);

/**
 * @brief   Retrieve the current state
 *
 * @param      ctx       Decoder context
 */
const char *vcdiff_state_str (vcdiff_t *ctx);

/**
 * @brief   Retrieve the an error messages after a nun-zero return code
 *
 * @param      ctx       Decoder context
 */
const char *vcdiff_error_str (vcdiff_t *ctx);

#endif
/** @} */
