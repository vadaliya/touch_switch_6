/*
 * rgb_thread_entry.c
 *
 * FSP-generated once for the "RGB Thread" (symbol: rgb_thread).
 *
 * Consumes rgb_update_t requests from any application manager and
 * drives the 11 backlight LEDs + 1 Wi-Fi status LED accordingly (two
 * independent Neopixel chains -- see rgb_led_driver.c). Also owns the
 * periodic blink timing since blink state needs to keep animating
 * even with no new update requests coming in.
 */

#include "app/app_common.h"
#include "drivers/rgb_led_driver.h"

#define RGB_BLINK_TICK_MS   (50u)   /* fine enough to service both 100ms and 500ms patterns */

void rgb_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    rgb_led_driver_init();

    for (;;)
    {
        rgb_update_t update;

        /* QUEUE: drain any pending update requests without blocking --
         * blink animation must keep running even with an empty queue.
         */
        while (xQueueReceive(g_rgb_update_queue, &update, 0) == pdTRUE)
        {
            /* MUTEX: brightness/color are shared config values that
             * backlight_manager (not yet built) will also write from
             * another thread -- take the mutex for the brief read,
             * release immediately.
             */
            if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                /* TODO once backlight_manager exists: read current
                 * on_color/off_color from the shared config struct
                 * into locals here, then dispatch on update.type:
                 *
                 *   RGB_UPDATE_SWITCH_STATE    -> rgb_led_driver_set_switch_led(update.param, ..., on_color, off_color)
                 *   RGB_UPDATE_FAN_SPEED       -> rgb_led_driver_set_fan_speed_leds(update.param, on_color, off_color)
                 *   RGB_UPDATE_WIFI_STATUS     -> rgb_led_driver_set_wifi_pattern((wifi_led_pattern_t) update.param, on_color)
                 *   RGB_UPDATE_GLOBAL_COLOR    -> re-apply current switch/fan states with new colors
                 *   RGB_UPDATE_BRIGHTNESS      -> rgb_led_driver_set_brightness(update.param)
                 *   RGB_UPDATE_BACKLIGHT_ONOFF -> rgb_led_driver_set_backlight_enable(update.param != 0)
                 *   RGB_UPDATE_IR_LEARN_BLINK  -> rgb_led_driver_start_blink(update.led_mask, on_color, 500)
                 */
                xSemaphoreGive(g_config_mutex);
            }
        }

        /* Advances any active blink timers (generic backlight blink
         * groups + Wi-Fi pattern) and pushes updated pixel data to
         * both LED chains.
         */
        rgb_led_driver_tick(RGB_BLINK_TICK_MS);

        /* EVENT GROUP: heartbeat check-in */
        xEventGroupSetBits(g_heartbeat_event_group, HEARTBEAT_BIT_RGB);

        vTaskDelay(pdMS_TO_TICKS(RGB_BLINK_TICK_MS));
    }
}
