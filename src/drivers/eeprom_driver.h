/*
 * eeprom_driver.h
 *
 * Sits on hal_flash, adding the one thing hal_flash deliberately
 * doesn't do: serializing access so only one write/erase is ever in
 * flight, via g_eeprom_mutex (already created via the Configurator
 * wizard per this project's established IPC pattern).
 *
 * Two full API sets, matching the request: BLOCKING (simple, waits
 * for completion before returning -- use this unless you have a
 * specific reason not to) and NON-BLOCKING/ASYNC (starts the
 * operation and returns immediately -- use when the calling thread
 * genuinely cannot afford to block, e.g. inside a time-sensitive loop).
 *
 * *** USAGE CONTRACT FOR THE ASYNC API ***
 * The task that calls eeprom_write_async_start()/eeprom_erase_async_start()
 * MUST be the same task that later calls the matching _poll() or
 * _wait() function. g_eeprom_mutex is held across the whole async
 * operation and released by whichever of those two you call once
 * completion is detected -- if you never poll/wait, the mutex stays
 * locked forever and every future EEPROM access (blocking or
 * non-blocking, from any thread) will hang or time out.
 */

#ifndef EEPROM_DRIVER_H
#define EEPROM_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * eeprom_driver_init
 * Opens the underlying flash peripheral. Call once at startup.
 */
void eeprom_driver_init(void);

/* --------------------------------------------------------------------- */
/* Read -- always synchronous (data flash reads are memory-mapped and    */
/* fast; there's no async variant to offer). Still serialized against    */
/* any in-flight write/erase via the mutex, since reading data flash     */
/* while it's mid-erase/write is not reliable on this hardware.          */
/* --------------------------------------------------------------------- */

/**
 * eeprom_read
 * @return  true on success, false if the mutex couldn't be acquired
 *          within timeout_ms (i.e. a write/erase was in progress and
 *          didn't finish in time).
 */
bool eeprom_read(uint32_t offset, uint8_t *dst, uint32_t length, uint32_t timeout_ms);

/* --------------------------------------------------------------------- */
/* BLOCKING API                                                           */
/* --------------------------------------------------------------------- */

/**
 * eeprom_write_blocking
 * Acquires exclusive access, writes, waits for completion, releases.
 * @return  false if the mutex or the flash operation itself timed out.
 */
bool eeprom_write_blocking(uint32_t offset, const uint8_t *data, uint32_t length, uint32_t timeout_ms);

/**
 * eeprom_erase_blocking
 * Same shape as eeprom_write_blocking, for erasing num_blocks starting
 * at offset (must be block-aligned -- see FLASH_DATA_BLOCK_SIZE).
 */
bool eeprom_erase_blocking(uint32_t offset, uint32_t num_blocks, uint32_t timeout_ms);

/* --------------------------------------------------------------------- */
/* NON-BLOCKING / ASYNC API -- see usage contract at the top of this     */
/* file before using these.                                              */
/* --------------------------------------------------------------------- */

/**
 * eeprom_write_async_start
 *
 * Attempts to acquire exclusive access WITHOUT blocking (0 timeout on
 * the mutex) and, if successful, starts the write and returns
 * immediately.
 *
 * @return  true if the write was started (caller now owns the
 *          obligation to poll/wait for completion -- see contract
 *          above). false if another operation is already in flight
 *          (mutex busy) -- nothing was started, no cleanup needed.
 */
bool eeprom_write_async_start(uint32_t offset, const uint8_t *data, uint32_t length);

/**
 * eeprom_erase_async_start
 * Same shape as eeprom_write_async_start, for erasing.
 */
bool eeprom_erase_async_start(uint32_t offset, uint32_t num_blocks);

/**
 * eeprom_async_poll
 *
 * Non-blocking check on whichever async operation was most recently
 * started. If it has completed, releases the mutex and returns true.
 * If still in progress, returns false (mutex remains held -- call
 * this again later).
 */
bool eeprom_async_poll(void);

/**
 * eeprom_async_wait
 *
 * Blocking wait (up to timeout_ms) for the most recently started async
 * operation to complete. Releases the mutex and returns true on
 * completion; returns false (mutex STILL held) on timeout -- the
 * operation is presumably still in progress, so releasing early would
 * violate the "only one operation in flight" invariant.
 */
bool eeprom_async_wait(uint32_t timeout_ms);

#endif /* EEPROM_DRIVER_H */