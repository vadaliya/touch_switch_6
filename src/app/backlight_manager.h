/*
 * backlight_manager.h
 *
 * Backlight configuration and RGB LED update dispatch manager.
 * Manages global backlight enable, brightness, ON/OFF colors, and
 * EEPROM persistence at offset 0x100.
 */

#ifndef BACKLIGHT_MANAGER_H
#define BACKLIGHT_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "drivers/rgb_led_driver.h"

#define BACKLIGHT_EEPROM_OFFSET   (0x100u)
#define BACKLIGHT_MAGIC           (0xB5u)

typedef struct
{
    uint8_t     magic;        /* 0xB5 */
    bool        enabled;
    uint8_t     brightness;   /* 0..100 % */
    rgb_color_t on_color;     /* Color when output is ON */
    rgb_color_t off_color;    /* Color when output is OFF */
    uint8_t     checksum;
} backlight_config_t;

/**
 * backlight_manager_init
 * Loads backlight settings from EEPROM or applies defaults.
 */
void backlight_manager_init(void);

/**
 * Getters (Thread-safe via g_config_mutex)
 */
bool    backlight_manager_get_enabled(void);
uint8_t backlight_manager_get_brightness(void);
void    backlight_manager_get_on_color(rgb_color_t *out_color);
void    backlight_manager_get_off_color(rgb_color_t *out_color);
void    backlight_manager_get_colors(rgb_color_t *out_on_color, rgb_color_t *out_off_color);

/**
 * Setters (Thread-safe, posts RGB update and persists to EEPROM)
 */
void backlight_manager_set_enabled(bool enabled);
void backlight_manager_set_brightness(uint8_t percent_0to100);
void backlight_manager_set_on_color(const rgb_color_t *color);
void backlight_manager_set_off_color(const rgb_color_t *color);

/**
 * Remote R2 / Convenience actions (Thread-safe, persist and dispatch)
 */
void backlight_manager_toggle_enabled(void);
void backlight_manager_brightness_inc(void);
void backlight_manager_brightness_dec(void);
void backlight_manager_cycle_on_color(void);
void backlight_manager_cycle_off_color(void);

/**
 * Notification helpers to post updates to g_rgb_update_queue
 */
void backlight_manager_notify_switch_state(uint8_t switch_id, bool on);
void backlight_manager_notify_wifi_status(wifi_led_pattern_t pattern);
void backlight_manager_refresh_all(void);

#endif /* BACKLIGHT_MANAGER_H */

