/*
 * hal_uart.h
 *
 * Thin wrapper over FSP's r_sci_uart driver (g_uart_qe / SCI9 on P109 TX / P110 RX)
 * dedicated to the Tuya Wi-Fi module link. SCI0 (g_uart0 on P101/P104) is dedicated
 * to text debug logging via app_log.c.
 *
 * When QE_TUNNING_TESTING is enabled (1), g_uart_qe routes its callback to QE Touch tuning.
 * When QE_TUNNING_TESTING is disabled (0), g_uart_qe routes its callback to Tuya MCU SDK.
 *
 * RX uses UART_EVENT_RX_CHAR (byte-by-byte, no prior length known) -- each received byte
 * lands in a small ring buffer, and tuya_uart_thread_entry.c drains it after waking on
 * g_uart_rx_semaphore.
 *
 * TX has both blocking and non-blocking entry points.
 */

#ifndef HAL_UART_H
#define HAL_UART_H

#include "common_data.h" /* FSP-generated: g_uart_qe, g_uart_rx_semaphore, g_uart_tx_complete_semaphore */
#include <stdint.h>
#include <stdbool.h>

#define UART_RX_RING_BUFFER_SIZE   (128u)

/**
 * hal_uart_callback
 * UART interrupt callback for Tuya Wi-Fi module.
 */
void hal_uart_callback(uart_callback_args_t *p_args);

/**
 * hal_uart_init
 * Configures g_uart_qe (SCI9) callback based on QE_TUNNING_TESTING macro and resets RX ring buffer.
 */
void hal_uart_init(void);

/**
 * hal_uart_send_blocking
 * Starts transmission and waits (up to timeout_ms) for it to complete.
 * @return  false if transmission couldn't be started, or timed out.
 */
bool hal_uart_send_blocking(const uint8_t *data, uint32_t length, uint32_t timeout_ms);

/**
 * hal_uart_send_start / hal_uart_send_wait_complete / hal_uart_send_poll_complete
 * Non-blocking send -- same start/poll/wait shape as hal_flash.h.
 * NOTE: unlike eeprom_driver's mutex-guarded async API, this project
 * doesn't yet have a dedicated TX-serialization primitive here -- if
 * you have multiple threads sending Tuya frames, add a mutex around
 * these calls the same way eeprom_driver wraps hal_flash (out of
 * scope for this HAL-layer file; that belongs in the future
 * tuya_mcu_sdk middleware layer, which is expected to be the only
 * caller of TX in practice).
 */
bool hal_uart_send_start(const uint8_t *data, uint32_t length);
bool hal_uart_send_wait_complete(uint32_t timeout_ms);
bool hal_uart_send_poll_complete(void);

/**
 * hal_uart_rx_available
 * @return  number of bytes currently sitting in the RX ring buffer.
 */
uint32_t hal_uart_rx_available(void);

/**
 * hal_uart_rx_read
 * Copies up to max_length bytes out of the RX ring buffer into dst.
 * @return  number of bytes actually copied (may be less than
 *          max_length if fewer bytes were available).
 */
uint32_t hal_uart_rx_read(uint8_t *dst, uint32_t max_length);

#endif /* HAL_UART_H */