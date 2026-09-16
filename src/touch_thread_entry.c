/*
 * touch_thread_entry.c
 *
 * FSP-generated once for the "Touch Thread" (symbol: touch_thread),
 * then left alone on future regenerations -- write the task body here.
 *
 * Periodically triggers an rm_touch scan (Touch middleware, calibrated
 * via QE for Capacitive Touch) covering the 7 buttons + 4 slider
 * zones, and posts touch_event_t items to g_touch_event_queue for the
 * application-layer managers to consume. See touch_driver.c for the
 * long-press-timing and slider-zone-transition logic -- debounce and
 * touch thresholding themselves are handled by rm_touch, not here.
 */

#include "app/app_common.h"
#include "drivers/touch_driver.h"

#define TOUCH_SCAN_PERIOD_MS   (20u)   /* trigger rate for hal_touch_scan_and_wait();
                                          keep in sync with any related timing elsewhere */

void touch_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    TickType_t last_wake_time = xTaskGetTickCount();

    touch_driver_init();

    for (;;)
    {
        touch_driver_scan();

        /* EVENT GROUP: heartbeat check-in -- tells System thread this
         * thread completed a healthy iteration since the last check.
         */
        xEventGroupSetBits(g_heartbeat_event_group, HEARTBEAT_BIT_TOUCH);

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(TOUCH_SCAN_PERIOD_MS));
    }
}