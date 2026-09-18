/*
 * rgb_led_driver.c
 *
 * Driver for 6-switch module Neopixel LEDs (Backlight on P400, Wi-Fi on P401).
 */

#include "drivers/rgb_led_driver.h"
#include "hal/hal_neopixel.h"
#include "hal_data.h"
#include "app/app_log.h"

#include <string.h>

#define MAX_BLINK_GROUPS   (4u)

static const bsp_io_port_pin_t BACKLIGHT_DATA_PIN = FSP_BACKLIGHT_DATA_PIN; /* P400, physical pin 1 */
static const bsp_io_port_pin_t WIFI_DATA_PIN      = FSP_WIFI_DATA_PIN;      /* P401, physical pin 2 */

/* switch_id (1-6) -> backlight LED index per confirmed wiring:
 * LED0 = SW1, LED1 = SW3, LED2 = SW5, LED3 = SW6, LED4 = SW4, LED5 = SW2.
 * Index 0 unused (switch_id is 1-based).
 */
static const uint8_t s_switch_to_led[7] = { 0xFFu, 0u, 5u, 1u, 4u, 2u, 3u };

static rgb_color_t s_backlight_raw[BACKLIGHT_LED_COUNT];
static rgb_color_t s_wifi_raw;

static uint8_t s_brightness_percent = 100u;
static bool    s_backlight_enabled  = true;

typedef struct
{
    bool        active;
    uint16_t    led_mask;
    rgb_color_t color;
    uint16_t    period_ms;
    uint32_t    elapsed_ms;
    bool        phase_on;
    uint8_t     toggles_remaining;
} blink_group_t;

static blink_group_t s_blink_group[MAX_BLINK_GROUPS];

typedef struct
{
    wifi_led_pattern_t pattern;
    rgb_color_t        color;
    uint32_t           elapsed_ms;
    bool               phase_on;
} wifi_pattern_state_t;

static wifi_pattern_state_t s_wifi_state = { WIFI_LED_OFF, { 0, 0, 0 }, 0u, false };

static uint8_t scale_channel(uint8_t raw, uint8_t percent)
{
    return (uint8_t) (((uint16_t) raw * percent) / 100u);
}

static void write_grb(uint8_t *dest, rgb_color_t color, uint8_t brightness_percent)
{
    dest[0] = scale_channel(color.g, brightness_percent);
    dest[1] = scale_channel(color.r, brightness_percent);
    dest[2] = scale_channel(color.b, brightness_percent);
}

void rgb_led_driver_init(void)
{
    memset(s_backlight_raw, 0, sizeof(s_backlight_raw));
    memset(&s_wifi_raw, 0, sizeof(s_wifi_raw));
    memset(s_blink_group, 0, sizeof(s_blink_group));

    s_brightness_percent = 100u;
    s_backlight_enabled  = true;

    s_wifi_state.pattern    = WIFI_LED_OFF;
    s_wifi_state.color      = (rgb_color_t) { 0u, 0u, 0u };
    s_wifi_state.elapsed_ms = 0u;
    s_wifi_state.phase_on   = false;

    hal_neopixel_init(BACKLIGHT_DATA_PIN);
    hal_neopixel_init(WIFI_DATA_PIN);

    rgb_led_driver_show();
}

void rgb_led_driver_set_switch_led(uint8_t switch_id, bool on, rgb_color_t on_color, rgb_color_t off_color)
{
    if ((switch_id < 1u) || (switch_id > 6u))
    {
        return;
    }

    uint8_t led_index = s_switch_to_led[switch_id];
    if (led_index < BACKLIGHT_LED_COUNT)
    {
        s_backlight_raw[led_index] = on ? on_color : off_color;
    }
}

uint8_t rgb_led_driver_switch_led_bit(uint8_t switch_id)
{
    if ((switch_id < 1u) || (switch_id > 6u))
    {
        return 0u;
    }
    return (uint8_t) (1u << s_switch_to_led[switch_id]);
}

void rgb_led_driver_set_wifi_pattern(wifi_led_pattern_t pattern, rgb_color_t color)
{
    s_wifi_state.pattern    = pattern;
    s_wifi_state.color      = color;
    s_wifi_state.elapsed_ms = 0u;
    s_wifi_state.phase_on   = (pattern == WIFI_LED_OFF) ? false : true;
}

void rgb_led_driver_start_blink(uint16_t led_mask, rgb_color_t blink_color, uint16_t period_ms)
{
    rgb_led_driver_start_blink_pulses(led_mask, blink_color, period_ms, 0u);
}

void rgb_led_driver_start_blink_pulses(uint16_t led_mask, rgb_color_t blink_color, uint16_t period_ms, uint8_t pulse_count)
{
    int8_t target = -1;

    for (uint8_t i = 0u; i < MAX_BLINK_GROUPS; i++)
    {
        if (s_blink_group[i].active && (s_blink_group[i].led_mask == led_mask))
        {
            target = (int8_t) i;
            break;
        }
    }

    if (target < 0)
    {
        for (uint8_t i = 0u; i < MAX_BLINK_GROUPS; i++)
        {
            if (!s_blink_group[i].active)
            {
                target = (int8_t) i;
                break;
            }
        }
    }

    if (target < 0)
    {
        target = 0;
    }

    s_blink_group[target].active            = true;
    s_blink_group[target].led_mask          = led_mask;
    s_blink_group[target].color             = blink_color;
    s_blink_group[target].period_ms         = period_ms;
    s_blink_group[target].elapsed_ms        = 0u;
    s_blink_group[target].phase_on          = true;
    s_blink_group[target].toggles_remaining = (uint8_t) (pulse_count * 2u);
}

void rgb_led_driver_stop_blink(uint16_t led_mask)
{
    for (uint8_t i = 0u; i < MAX_BLINK_GROUPS; i++)
    {
        if (s_blink_group[i].active && (s_blink_group[i].led_mask == led_mask))
        {
            s_blink_group[i].active = false;
        }
    }
}

void rgb_led_driver_set_brightness(uint8_t percent_0_100)
{
    s_brightness_percent = (percent_0_100 > 100u) ? 100u : percent_0_100;
}

void rgb_led_driver_set_backlight_enable(bool enabled)
{
    s_backlight_enabled = enabled;
}

void rgb_led_driver_show(void)
{
    uint8_t backlight_grb[BACKLIGHT_LED_COUNT * 3u];
    uint8_t wifi_grb[WIFI_LED_COUNT * 3u];

    if (!s_backlight_enabled)
    {
        memset(backlight_grb, 0, sizeof(backlight_grb));
        memset(wifi_grb, 0, sizeof(wifi_grb));
    }
    else
    {
        for (uint8_t i = 0u; i < BACKLIGHT_LED_COUNT; i++)
        {
            rgb_color_t color = s_backlight_raw[i];

            for (uint8_t g = 0u; g < MAX_BLINK_GROUPS; g++)
            {
                if (s_blink_group[g].active && ((s_blink_group[g].led_mask >> i) & 0x1u))
                {
                    color = s_blink_group[g].phase_on
                                ? s_blink_group[g].color
                                : (rgb_color_t) { 0, 0, 0 };
                }
            }

            write_grb(&backlight_grb[i * 3u], color, s_brightness_percent);
        }

        rgb_color_t wifi_color = s_wifi_state.phase_on ? s_wifi_state.color : (rgb_color_t) { 0, 0, 0 };
        write_grb(wifi_grb, wifi_color, 100u);
    }

    hal_neopixel_transmit(BACKLIGHT_DATA_PIN, backlight_grb, BACKLIGHT_LED_COUNT);
    hal_neopixel_transmit(WIFI_DATA_PIN, wifi_grb, WIFI_LED_COUNT);
}

void rgb_led_driver_tick(uint32_t elapsed_ms)
{
    for (uint8_t i = 0u; i < MAX_BLINK_GROUPS; i++)
    {
        if (!s_blink_group[i].active)
        {
            continue;
        }

        s_blink_group[i].elapsed_ms += elapsed_ms;

        if (s_blink_group[i].elapsed_ms >= s_blink_group[i].period_ms)
        {
            s_blink_group[i].elapsed_ms = 0u;
            s_blink_group[i].phase_on   = !s_blink_group[i].phase_on;

            if (s_blink_group[i].toggles_remaining > 0u)
            {
                s_blink_group[i].toggles_remaining--;
                if (s_blink_group[i].toggles_remaining == 0u)
                {
                    s_blink_group[i].active = false;
                }
            }
        }
    }

    if (s_wifi_state.pattern == WIFI_LED_PERIODIC_SINGLE_BLINK)
    {
        #define PERIODIC_BLINK_WINDOW_MS   (60000u)
        s_wifi_state.elapsed_ms += elapsed_ms;

        if (s_wifi_state.elapsed_ms >= PERIODIC_BLINK_WINDOW_MS)
        {
            s_wifi_state.elapsed_ms = 0u;
            s_wifi_state.phase_on   = true;
        }
        else if (s_wifi_state.elapsed_ms >= elapsed_ms)
        {
            s_wifi_state.phase_on = false;
        }
    }
    else if ((s_wifi_state.pattern == WIFI_LED_FAST_BLINK) || (s_wifi_state.pattern == WIFI_LED_SLOW_BLINK))
    {
        uint16_t period_ms = (s_wifi_state.pattern == WIFI_LED_FAST_BLINK) ? 100u : 500u;
        s_wifi_state.elapsed_ms += elapsed_ms;

        if (s_wifi_state.elapsed_ms >= period_ms)
        {
            s_wifi_state.elapsed_ms = 0u;
            s_wifi_state.phase_on   = !s_wifi_state.phase_on;
        }
    }
    else
    {
        s_wifi_state.phase_on = false;
    }

    rgb_led_driver_show();
}