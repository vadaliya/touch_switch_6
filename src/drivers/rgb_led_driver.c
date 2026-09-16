/*
 * rgb_led_driver.c
 *
 * See rgb_led_driver.h.
 */

#include "drivers/rgb_led_driver.h"
#include "hal/hal_neopixel.h"
#include "hal_data.h"

#include <string.h>

#define WIFI_LED_COUNT           (1u)
#define MAX_BLINK_GROUPS         (2u)  /* concurrent independent blink masks on the backlight chain */

static const bsp_io_port_pin_t BACKLIGHT_DATA_PIN = BSP_IO_PORT_04_PIN_00; /* P400, physical pin 1 */
static const bsp_io_port_pin_t WIFI_DATA_PIN      = BSP_IO_PORT_04_PIN_01; /* P401, physical pin 2 */

/* switch_id (1-6) -> backlight LED index, per spec's "RGB LED
 * Arrangement for Touch" table: S1->LED0, S2->LED10, S3->LED1,
 * S4->LED9, S5->LED2, S6->LED8. Index 0 unused (switch_id is 1-based).
 */
static const uint8_t s_switch_to_led[7] = { 0xFFu, 0u, 10u, 1u, 9u, 2u, 8u };

#define LED_FAN_ONOFF   (3u)
#define LED_FAN_SPEED4  (4u)
#define LED_FAN_SPEED3  (5u)
#define LED_FAN_SPEED2  (6u)
#define LED_FAN_SPEED1  (7u)

/* Raw (unscaled) colors -- brightness is applied only at transmit
 * time, so repeated brightness changes never degrade the stored color. */
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
} blink_group_t;

static blink_group_t s_blink_group[MAX_BLINK_GROUPS];

typedef struct
{
    wifi_led_pattern_t pattern;
    rgb_color_t         color;
    uint32_t             elapsed_ms;
    bool                  phase_on;
} wifi_pattern_state_t;

static wifi_pattern_state_t s_wifi_state = { WIFI_LED_OFF, { 0, 0, 0 }, 0u, false };

/* --------------------------------------------------------------------- */
/* Helpers                                                                */
/* --------------------------------------------------------------------- */

static uint8_t scale_channel(uint8_t raw, uint8_t percent)
{
    return (uint8_t) (((uint16_t) raw * percent) / 100u);
}

static void write_grb(uint8_t *dest, rgb_color_t color, uint8_t brightness_percent)
{
    /* WS2812 wire order is GRB, not RGB. */
    dest[0] = scale_channel(color.g, brightness_percent);
    dest[1] = scale_channel(color.r, brightness_percent);
    dest[2] = scale_channel(color.b, brightness_percent);
}

/* --------------------------------------------------------------------- */
/* Public API                                                             */
/* --------------------------------------------------------------------- */

void rgb_led_driver_init(void)
{
    memset(s_backlight_raw, 0, sizeof(s_backlight_raw));
    memset(&s_wifi_raw, 0, sizeof(s_wifi_raw));
    memset(s_blink_group, 0, sizeof(s_blink_group));

    s_brightness_percent = 100u;
    s_backlight_enabled  = true;
    s_wifi_state.pattern = WIFI_LED_OFF;

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
    s_backlight_raw[led_index] = on ? on_color : off_color;
}

void rgb_led_driver_set_fan_onoff_led(bool on, rgb_color_t on_color, rgb_color_t off_color)
{
    s_backlight_raw[LED_FAN_ONOFF] = on ? on_color : off_color;
}

void rgb_led_driver_set_fan_speed_leds(uint8_t fan_speed_0to4, rgb_color_t on_color, rgb_color_t off_color)
{
    bool led4 = false, led5 = false, led6 = false, led7 = false;

    switch (fan_speed_0to4)
    {
        case 1: led7 = true; break;
        case 2: led7 = true; led6 = true; break;
        case 3: led7 = true; led6 = true; led5 = true; break;
        case 4: led7 = true; led6 = true; led5 = true; led4 = true; break;
        default: break; /* 0 / OFF / out-of-range: all stay off */
    }

    s_backlight_raw[LED_FAN_SPEED4] = led4 ? on_color : off_color;
    s_backlight_raw[LED_FAN_SPEED3] = led5 ? on_color : off_color;
    s_backlight_raw[LED_FAN_SPEED2] = led6 ? on_color : off_color;
    s_backlight_raw[LED_FAN_SPEED1] = led7 ? on_color : off_color;
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
    /* Reuse an existing group already using this exact mask, else find
     * a free slot, else overwrite slot 0 as a last resort (better to
     * repurpose an existing blink than silently drop the request). */
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
        target = 0; /* all slots busy -- repurpose the oldest */
    }

    s_blink_group[target].active     = true;
    s_blink_group[target].led_mask   = led_mask;
    s_blink_group[target].color      = blink_color;
    s_blink_group[target].period_ms  = period_ms;
    s_blink_group[target].elapsed_ms = 0u;
    s_blink_group[target].phase_on   = true;
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
    /* Caller is responsible for re-setting these LEDs' solid color via
     * rgb_led_driver_set_switch_led()/etc. afterward -- this function
     * only stops the blink animation, it doesn't know what "resting
     * state" color to restore. */
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

            /* Apply any active blink override for this LED. */
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
        /* Wi-Fi LED intentionally NOT scaled by s_brightness_percent --
         * see header comment: it's a status indicator, not part of the
         * dimmable backlight system. */
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
        }
    }

    if (s_wifi_state.pattern == WIFI_LED_PERIODIC_SINGLE_BLINK)
    {
        /* One brief blink every 60s, distinct from a continuous
         * symmetric blink -- on for one tick's worth (~50ms, whatever
         * elapsed_ms is), off for the rest of the 60s window. */
        #define PERIODIC_BLINK_WINDOW_MS   (60000u)
        s_wifi_state.elapsed_ms += elapsed_ms;

        if (s_wifi_state.elapsed_ms >= PERIODIC_BLINK_WINDOW_MS)
        {
            s_wifi_state.elapsed_ms = 0u;
            s_wifi_state.phase_on   = true; /* start of window: blink on */
        }
        else if (s_wifi_state.elapsed_ms >= elapsed_ms)
        {
            /* Past the first tick of the window -- blink pulse over. */
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
    else /* WIFI_LED_OFF */
    {
        s_wifi_state.phase_on = false;
    }

    rgb_led_driver_show();
}