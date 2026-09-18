/*
 * rgb_led_driver.h
 *
 * Driver for 6-switch module Neopixel LEDs:
 *   - P400: 6-LED backlight chain (LED0-LED5)
 *   - P401: 1-LED Wi-Fi status chain
 */

#ifndef RGB_LED_DRIVER_H
#define RGB_LED_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#define BACKLIGHT_LED_COUNT   (6u)
#define WIFI_LED_COUNT        (1u)

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

typedef enum
{
    WIFI_LED_OFF = 0,
    WIFI_LED_FAST_BLINK,           /* Smart Config: 100 ms period */
    WIFI_LED_SLOW_BLINK,           /* AP Mode: 500 ms period */
    WIFI_LED_PERIODIC_SINGLE_BLINK /* No Wi-Fi: one blink every 60s */
} wifi_led_pattern_t;

/**
 * rgb_led_driver_init
 * Configures both data pins (P400, P401) and clears both chains to off.
 */
void rgb_led_driver_init(void);

/**
 * rgb_led_driver_set_switch_led
 * @param switch_id  1-6 (SW1-SW6) -- internally mapped to physical LED index:
 *                    LED0=SW1, LED1=SW3, LED2=SW5, LED3=SW6, LED4=SW4, LED5=SW2.
 */
void rgb_led_driver_set_switch_led(uint8_t switch_id, bool on, rgb_color_t on_color, rgb_color_t off_color);

/**
 * rgb_led_driver_switch_led_bit
 * Returns the backlight chain bit (1u << physical_index) for a given switch_id (1-6),
 * or 0 for an out-of-range switch_id.
 */
uint8_t rgb_led_driver_switch_led_bit(uint8_t switch_id);

/**
 * rgb_led_driver_set_wifi_pattern
 * Sets the target blink pattern for the Wi-Fi status chain.
 */
void rgb_led_driver_set_wifi_pattern(wifi_led_pattern_t pattern, rgb_color_t color);

/**
 * rgb_led_driver_start_blink / rgb_led_driver_stop_blink
 * Generic blink group for the backlight chain (used for IR learning indication).
 * led_mask bit i = LED i (0..5).
 */
void rgb_led_driver_start_blink(uint16_t led_mask, rgb_color_t blink_color, uint16_t period_ms);
void rgb_led_driver_start_blink_pulses(uint16_t led_mask, rgb_color_t blink_color, uint16_t period_ms, uint8_t pulse_count);
void rgb_led_driver_stop_blink(uint16_t led_mask);

/**
 * rgb_led_driver_set_brightness
 * @param percent_0_100  applied to the backlight chain at transmit time.
 */
void rgb_led_driver_set_brightness(uint8_t percent_0_100);

/**
 * rgb_led_driver_set_backlight_enable
 * Master on/off. Disabling turns off all backlight and Wi-Fi LEDs.
 */
void rgb_led_driver_set_backlight_enable(bool enabled);

/**
 * rgb_led_driver_show
 * Pushes the current buffered state of both chains out over the wire.
 */
void rgb_led_driver_show(void);

/**
 * rgb_led_driver_tick
 * Advances active blink timers and calls rgb_led_driver_show().
 */
void rgb_led_driver_tick(uint32_t elapsed_ms);

#endif /* RGB_LED_DRIVER_H */