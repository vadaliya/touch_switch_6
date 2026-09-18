/*
 * rgb_thread_entry.c
 *
 * FSP-generated once for the "RGB Thread" (symbol: rgb_thread).
 *
 * Consumes rgb_update_t requests from any application manager and
 * drives the 6-switch backlight chain (P400) and Wi-Fi status LED (P401)
 * accordingly (two independent Neopixel chains -- see rgb_led_driver.c).
 * Also owns the periodic blink timing since blink state needs to keep
 * animating even with no new update requests coming in.
 */

#include "app/app_common.h"
#include "app/backlight_manager.h"
#include "app/light_manager.h"
#include "drivers/rgb_led_driver.h"

#define RGB_BLINK_TICK_MS   (50u)   /* fine enough to service both 100ms and 500ms patterns */

extern void rgb_thread_entry(void *pvParameters);

void rgb_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    rgb_led_driver_init();
    backlight_manager_init();

    LOG_INFO("RGB thread started");

    /* Initial state sync */
    backlight_manager_refresh_all();

    for (;;)
    {
        rgb_update_t update;

        /* QUEUE: drain any pending update requests without blocking --
         * blink animation must keep running even with an empty queue.
         */
        while (xQueueReceive(g_rgb_update_queue, &update, 0) == pdTRUE)
        {
            rgb_color_t on_color;
            rgb_color_t off_color;
            backlight_manager_get_colors(&on_color, &off_color);

            switch (update.type)
            {
                case RGB_UPDATE_SWITCH_STATE:
                    rgb_led_driver_set_switch_led(update.param, update.led_mask != 0u, on_color, off_color);
                    break;

                case RGB_UPDATE_WIFI_STATUS:
                {
                    rgb_color_t wifi_color = on_color;
                    /* Ensure status LED has a visible non-zero color */
                    if ((0u == wifi_color.r) && (0u == wifi_color.g) && (0u == wifi_color.b))
                    {
                        wifi_color = (rgb_color_t) { 0u, 255u, 0u }; /* Fallback bright Green */
                    }
                    rgb_led_driver_set_wifi_pattern((wifi_led_pattern_t) update.param, wifi_color);
                    break;
                }

                case RGB_UPDATE_GLOBAL_COLOR:
                    for (uint8_t sw = 1u; sw <= LIGHT_SWITCH_COUNT; sw++)
                    {
                        rgb_led_driver_set_switch_led(sw, light_manager_get_switch(sw), on_color, off_color);
                    }
                    break;

                case RGB_UPDATE_BRIGHTNESS:
                    rgb_led_driver_set_brightness(update.param);
                    break;

                case RGB_UPDATE_BACKLIGHT_ONOFF:
                    rgb_led_driver_set_backlight_enable(update.param != 0u);
                    break;

                case RGB_UPDATE_IR_LEARN_BLINK:
                    rgb_led_driver_start_blink((uint16_t) update.led_mask, on_color, 500u);
                    break;

                default:
                    break;
            }
        }

        /* Advances any active blink timers (generic backlight blink
         * groups + Wi-Fi pattern) and pushes updated pixel data to
         * all three LED chains.
         */
        rgb_led_driver_tick(RGB_BLINK_TICK_MS);

        /* EVENT GROUP: heartbeat check-in */
        xEventGroupSetBits(g_heartbeat_event_group, HEARTBEAT_BIT_RGB);

        vTaskDelay(pdMS_TO_TICKS(RGB_BLINK_TICK_MS));
    }
}