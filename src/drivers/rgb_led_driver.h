/*
 * rgb_led_driver.h
 *
 * Two independent Neopixel (WS2812-style) chains, per the real PCB:
 *   - P400: 11-LED backlight chain (LED0-LED10, per spec's mapping)
 *   - P401: 1-LED Wi-Fi status chain
 *
 * The LED0-LED10 -> switch/fan-speed mapping below is a fixed wiring
 * fact from the spec's "RGB LED Arrangement for Touch" table -- same
 * category as relay_driver's pin map, so it lives here rather than in
 * an application-layer manager. What COLOR each state uses (on_color/
 * off_color) is configurable via the app (Tuya DP, IR remote) and is
 * NOT owned by this driver -- callers pass colors in explicitly.
 */

#ifndef RGB_LED_DRIVER_H
#define RGB_LED_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#define BACKLIGHT_LED_COUNT   (11u)

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

typedef enum
{
    WIFI_LED_OFF = 0,
    WIFI_LED_FAST_BLINK,          /* Smart Config: 100 ms period, per spec Timing Requirements */
    WIFI_LED_SLOW_BLINK,          /* AP Mode: 500 ms period */
    WIFI_LED_PERIODIC_SINGLE_BLINK /* No Wi-Fi: one blink every 60s, distinct from continuous slow blink */
} wifi_led_pattern_t;

/**
 * rgb_led_driver_init
 * Configures both data pins and clears both chains to off.
 */
void rgb_led_driver_init(void);

/**
 * rgb_led_driver_set_switch_led
 *
 * Maps switch_id (1-6) to its wired LED index per the spec table
 * (S1->LED0, S2->LED10, S3->LED1, S4->LED9, S5->LED2, S6->LED8) and
 * sets it to on_color or off_color. Does not transmit -- call
 * rgb_led_driver_show() after a batch of changes.
 */
void rgb_led_driver_set_switch_led(uint8_t switch_id, bool on, rgb_color_t on_color, rgb_color_t off_color);

/**
 * rgb_led_driver_set_fan_onoff_led
 * Maps to LED3 (Fan ON/OFF indicator).
 */
void rgb_led_driver_set_fan_onoff_led(bool on, rgb_color_t on_color, rgb_color_t off_color);

/**
 * rgb_led_driver_set_fan_speed_leds
 *
 * Sets LED4-LED7 per spec's cumulative pattern: Speed1 = LED7 only,
 * Speed2 = LED7+LED6, Speed3 = LED7+LED6+LED5, Speed4 = all four.
 * fan_speed is a plain 0-4 value here (this driver has no dependency
 * on relay_driver's fan_speed_t, since LED state and relay state are
 * independent on this board -- LEDs work even without fan relay
 * hardware present).
 */
void rgb_led_driver_set_fan_speed_leds(uint8_t fan_speed_0to4, rgb_color_t on_color, rgb_color_t off_color);

/**
 * rgb_led_driver_set_wifi_pattern
 * Sets the target blink pattern for the Wi-Fi status chain (P401).
 * Actual blinking is animated by rgb_led_driver_tick().
 */
void rgb_led_driver_set_wifi_pattern(wifi_led_pattern_t pattern, rgb_color_t color);

/**
 * rgb_led_driver_start_blink / rgb_led_driver_stop_blink
 *
 * Generic blink group for the BACKLIGHT chain only (used for IR
 * learning indication -- "the corresponding back-light LED shall
 * blink using the configured-ON color"). led_mask bit i = LED i.
 * Multiple calls with different masks are independent; overlapping
 * masks are not supported (last call wins for any shared bit).
 */
void rgb_led_driver_start_blink(uint16_t led_mask, rgb_color_t blink_color, uint16_t period_ms);
void rgb_led_driver_stop_blink(uint16_t led_mask);

/**
 * rgb_led_driver_set_brightness
 * @param percent_0_100  global brightness, applied to the BACKLIGHT
 *                        chain only at transmit time (Wi-Fi LED is
 *                        deliberately not dimmed -- it's a status
 *                        indicator, not part of the backlight system).
 */
void rgb_led_driver_set_brightness(uint8_t percent_0_100);

/**
 * rgb_led_driver_set_backlight_enable
 *
 * Master on/off. Per spec: "When the back-light function is disabled,
 * all RGB LEDs, including the fan speed indicator LEDs and Wi-Fi
 * status LED, shall turn OFF" -- so this affects BOTH chains, not
 * just the backlight one.
 */
void rgb_led_driver_set_backlight_enable(bool enabled);

/**
 * rgb_led_driver_show
 * Pushes the current buffered state of both chains out over the wire
 * (respecting brightness/backlight-enable). Call after one or more
 * set_* calls, or let rgb_led_driver_tick() call it for you each cycle.
 */
void rgb_led_driver_show(void);

/**
 * rgb_led_driver_tick
 * Advances all active blink timers (generic blink groups + Wi-Fi
 * pattern) by elapsed_ms and calls rgb_led_driver_show(). Call this
 * once per rgb_thread loop iteration.
 */
void rgb_led_driver_tick(uint32_t elapsed_ms);

#endif /* RGB_LED_DRIVER_H */
