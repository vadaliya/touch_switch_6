/*
 * tuya_uart_thread_entry.c
 *
 * FSP-generated once for the "Tuya UART Thread" (symbol: tuya_uart_thread).
 *
 * Owns the UART link to the Tuya Wi-Fi module (9600 baud, Tuya MCU
 * protocol). The SCI UART RX ISR (registered via the r_sci_uart callback
 * in the FSP Configurator) pushes received bytes into a ring buffer and
 * gives g_uart_rx_semaphore. This thread blocks on that semaphore,
 * reassembles frames via the vendored Tuya MCU SDK, and for each
 * decoded DP, posts a tuya_dp_cmd_t to g_tuya_dp_queue.
 */

#include "app/app_common.h"

void tuya_uart_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    /* TODO: hal_uart_init() for g_uart0 (SCI channel wired to the Tuya
     * module); tuya_mcu_sdk_init() to set up protocol state machine.
     * The UART callback should be minimal, e.g.:
     *
     *   void uart_callback(uart_callback_args_t *p_args)
     *   {
     *       if (p_args->event == UART_EVENT_RX_CHAR)
     *       {
     *           ring_buffer_push((uint8_t) p_args->data);
     *           BaseType_t hpt_woken = pdFALSE;
     *           xSemaphoreGiveFromISR(g_uart_rx_semaphore, &hpt_woken);
     *           portYIELD_FROM_ISR(hpt_woken);
     *       }
     *   }
     */

    for (;;)
    {
        /* SEMAPHORE: block until the UART RX ISR signals new bytes are
         * in the ring buffer. 1 s timeout keeps this thread's heartbeat
         * fresh even with the link idle.
         */
        if (xSemaphoreTake(g_uart_rx_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            /* TODO: tuya_mcu_sdk_process();
             *       -> drains the ring buffer, runs the Tuya frame state
             *          machine, and for each fully decoded DP:
             *
             *   tuya_dp_cmd_t cmd = { .dpid = dpid, .length = len };
             *   memcpy(cmd.value, payload, len);
             *   xQueueSend(g_tuya_dp_queue, &cmd, pdMS_TO_TICKS(20));
             *
             * Posting to a queue here (rather than calling
             * dp_download_*_handle() directly) keeps EEPROM writes
             * off this thread -- a slow flash write must never stall
             * UART servicing and risk dropping the next frame.
             */

            /* MUTEX example: if this thread ever needs to write EEPROM
             * directly (e.g. immediate ACK path), always go through the
             * mutex -- eeprom_driver calls from ANY thread must be
             * serialized since data flash write/erase are not reentrant:
             *
             *   if (xSemaphoreTake(g_eeprom_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
             *   {
             *       eeprom_driver_write(...);
             *       xSemaphoreGive(g_eeprom_mutex);
             *   }
             */
        }

        /* EVENT GROUP: heartbeat check-in */
        xEventGroupSetBits(g_heartbeat_event_group, HEARTBEAT_BIT_TUYA_UART);
    }
}
