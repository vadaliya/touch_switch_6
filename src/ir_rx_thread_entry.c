/*
 * ir_rx_thread_entry.c
 *
 * FSP-generated once for the "IR RX Thread" (symbol: ir_rx_thread).
 *
 * The actual RC5 edge capture happens in an ISR (input capture timer),
 * which records the edge timestamp and gives g_ir_capture_semaphore.
 * This thread blocks on that semaphore, runs the RC5 decode once enough
 * edges have accumulated, and if a valid code is found, posts it to
 * g_ir_event_queue.
 */

#include "app/app_common.h"

void ir_rx_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    /* TODO: ir_driver_init(); -- configure input capture timer + ICU interrupt.
     * The capture ISR itself (registered via the r_gpt/r_icu callback in
     * the FSP Configurator) should do the minimum possible work and then:
     *
     *   BaseType_t higher_priority_task_woken = pdFALSE;
     *   xSemaphoreGiveFromISR(g_ir_capture_semaphore, &higher_priority_task_woken);
     *   portYIELD_FROM_ISR(higher_priority_task_woken);
     */

    for (;;)
    {
        /* SEMAPHORE: block here until the capture ISR signals a new edge.
         * 1 s timeout means this thread still loops periodically even
         * with zero IR activity, which keeps the heartbeat bit fresh
         * without needing a separate polling path.
         */
        if (xSemaphoreTake(g_ir_capture_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            /* TODO: ir_driver_try_decode_rc5(&decoded_code);
             *       -> inspects accumulated edge timestamps; RC5 frames
             *          are short, so this may need to re-arm and wait
             *          for a few more semaphore gives before a full
             *          frame is available -- typical pattern is an
             *          internal edge count/state machine inside the
             *          driver, not "one semaphore give = one code".
             *
             *   uint16_t decoded_code;
             *   if (ir_driver_try_decode_rc5(&decoded_code))
             *   {
             *       ir_event_t evt = {
             *           .raw_code = decoded_code,
             *           .timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS
             *       };
             *       xQueueSend(g_ir_event_queue, &evt, 0);
             *   }
             */
        }

        /* EVENT GROUP: heartbeat check-in */
        xEventGroupSetBits(g_heartbeat_event_group, HEARTBEAT_BIT_IR_RX);
    }
}
