/*
 * backlight_manager.c
 *
 * Implements backlight configuration storage and RGB update queue helpers.
 */

#include "app/backlight_manager.h"
#include "app/app_common.h"
#include "drivers/eeprom_driver.h"
#include "app/app_log.h"

#include <string.h>

static backlight_config_t s_config = {
    .magic      = BACKLIGHT_MAGIC,
    .enabled    = true,
    .brightness = 100u,
    .on_color   = { 0u, 0u, 255u },      /* Blue (100%) */
    .off_color  = { 128u, 128u, 128u },  /* Dim White (50%) */
    .checksum   = 0u
};

static uint8_t calculate_checksum(const backlight_config_t *cfg)
{
    const uint8_t *bytes = (const uint8_t *) cfg;
    uint8_t csum = 0u;
    for (size_t i = 0u; i < (sizeof(backlight_config_t) - 1u); i++)
    {
        csum ^= bytes[i];
    }
    return csum;
}

static void persist_config(void)
{
    s_config.checksum = calculate_checksum(&s_config);

    /* Read existing IR codes & scenes (38 bytes) from 0x000 so they are preserved across Block 0 erase */
    uint8_t ir_buf[38];
    bool has_ir = eeprom_read(0u, ir_buf, sizeof(ir_buf), 100u);

    /* Read existing Power Recovery config (4 bytes) from 0x110 */
    uint8_t pr_buf[4];
    bool has_pr = eeprom_read(0x110u, pr_buf, sizeof(pr_buf), 100u);

    /* Read existing Master Switch config (4 bytes) from 0x120 */
    uint8_t ms_buf[4];
    bool has_ms = eeprom_read(0x120u, ms_buf, sizeof(ms_buf), 100u);

    if (eeprom_erase_blocking(0u, 1u, 2000u))
    {
        if (has_ir)
        {
            (void) eeprom_write_blocking(0u, ir_buf, sizeof(ir_buf), 1000u);
        }
        (void) eeprom_write_blocking(BACKLIGHT_EEPROM_OFFSET, (const uint8_t *) &s_config, sizeof(backlight_config_t), 1000u);
        if (has_pr)
        {
            (void) eeprom_write_blocking(0x110u, pr_buf, sizeof(pr_buf), 1000u);
        }
        if (has_ms && (ms_buf[0] == 0xD4u))
        {
            (void) eeprom_write_blocking(0x120u, ms_buf, sizeof(ms_buf), 1000u);
        }
    }
}

void backlight_manager_init(void)
{
    backlight_config_t loaded;
    if (eeprom_read(BACKLIGHT_EEPROM_OFFSET, (uint8_t *) &loaded, sizeof(backlight_config_t), 100u))
    {
        if ((loaded.magic == BACKLIGHT_MAGIC) &&
            (loaded.checksum == calculate_checksum(&loaded)) &&
            (loaded.brightness <= 100u))
        {
            if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                s_config = loaded;
                xSemaphoreGive(g_config_mutex);
            }
            LOG_INFO("Backlight config loaded: Brightness=%u%%, Enabled=%u",
                     s_config.brightness, s_config.enabled ? 1u : 0u);
            return;
        }
    }

    LOG_INFO("Backlight config uninitialized in EEPROM, writing defaults.");
    persist_config();
}

bool backlight_manager_get_enabled(void)
{
    bool val = true;
    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        val = s_config.enabled;
        xSemaphoreGive(g_config_mutex);
    }
    return val;
}

uint8_t backlight_manager_get_brightness(void)
{
    uint8_t val = 100u;
    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        val = s_config.brightness;
        xSemaphoreGive(g_config_mutex);
    }
    return val;
}

void backlight_manager_get_on_color(rgb_color_t *out_color)
{
    if (out_color == NULL)
    {
        return;
    }

    out_color->r = 0u;
    out_color->g = 255u;
    out_color->b = 0u;

    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        *out_color = s_config.on_color;
        xSemaphoreGive(g_config_mutex);
    }
}

void backlight_manager_get_off_color(rgb_color_t *out_color)
{
    if (out_color == NULL)
    {
        return;
    }

    out_color->r = 0u;
    out_color->g = 0u;
    out_color->b = 0u;

    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        *out_color = s_config.off_color;
        xSemaphoreGive(g_config_mutex);
    }
}

void backlight_manager_get_colors(rgb_color_t *out_on_color, rgb_color_t *out_off_color)
{
    if (out_on_color != NULL)
    {
        out_on_color->r = 0u;
        out_on_color->g = 255u;
        out_on_color->b = 0u;
    }

    if (out_off_color != NULL)
    {
        out_off_color->r = 0u;
        out_off_color->g = 0u;
        out_off_color->b = 0u;
    }

    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        if (out_on_color != NULL)
        {
            *out_on_color = s_config.on_color;
        }
        if (out_off_color != NULL)
        {
            *out_off_color = s_config.off_color;
        }
        xSemaphoreGive(g_config_mutex);
    }
}

void backlight_manager_set_enabled(bool enabled)
{
    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        s_config.enabled = enabled;
        persist_config();
        xSemaphoreGive(g_config_mutex);
    }

    rgb_update_t update = {
        .type     = RGB_UPDATE_BACKLIGHT_ONOFF,
        .param    = enabled ? 1u : 0u,
        .led_mask = 0u
    };
    xQueueSend(g_rgb_update_queue, &update, 0);
}

void backlight_manager_set_brightness(uint8_t percent_0to100)
{
    if (percent_0to100 > 100u)
    {
        percent_0to100 = 100u;
    }

    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        s_config.brightness = percent_0to100;
        persist_config();
        xSemaphoreGive(g_config_mutex);
    }

    rgb_update_t update = {
        .type     = RGB_UPDATE_BRIGHTNESS,
        .param    = percent_0to100,
        .led_mask = 0u
    };
    xQueueSend(g_rgb_update_queue, &update, 0);
}

void backlight_manager_set_on_color(const rgb_color_t *color)
{
    if (color == NULL)
    {
        return;
    }

    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        s_config.on_color = *color;
        persist_config();
        xSemaphoreGive(g_config_mutex);
    }

    backlight_manager_refresh_all();
}

void backlight_manager_set_off_color(const rgb_color_t *color)
{
    if (color == NULL)
    {
        return;
    }

    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        s_config.off_color = *color;
        persist_config();
        xSemaphoreGive(g_config_mutex);
    }

    backlight_manager_refresh_all();
}

void backlight_manager_notify_switch_state(uint8_t switch_id, bool on)
{
    rgb_update_t update = {
        .type     = RGB_UPDATE_SWITCH_STATE,
        .param    = switch_id,
        .led_mask = (uint16_t)(on ? 1u : 0u)
    };
    xQueueSend(g_rgb_update_queue, &update, 0);
}

void backlight_manager_notify_wifi_status(wifi_led_pattern_t pattern)
{
    rgb_update_t update = {
        .type     = RGB_UPDATE_WIFI_STATUS,
        .param    = (uint8_t) pattern,
        .led_mask = 0u
    };
    xQueueSend(g_rgb_update_queue, &update, 0);
}

void backlight_manager_refresh_all(void)
{
    rgb_update_t update = {
        .type     = RGB_UPDATE_GLOBAL_COLOR,
        .param    = 0u,
        .led_mask = 0u
    };
    xQueueSend(g_rgb_update_queue, &update, 0);
}

/* Preset Palettes for Remote R2 Color Cycling */
static const rgb_color_t s_on_palette[] = {
    { 0u, 0u, 255u },    /* Blue (Default) */
    { 0u, 255u, 0u },    /* Green */
    { 255u, 0u, 0u },    /* Red */
    { 0u, 255u, 255u },  /* Cyan */
    { 255u, 0u, 255u },  /* Magenta */
    { 255u, 255u, 0u },  /* Yellow */
    { 255u, 128u, 0u },  /* Orange */
    { 255u, 255u, 255u },/* White */
    { 255u, 180u, 50u }, /* Warm White */
    { 128u, 0u, 255u }   /* Purple */
};

#define ON_PALETTE_SIZE (sizeof(s_on_palette) / sizeof(s_on_palette[0]))

static const rgb_color_t s_off_palette[] = {
    { 128u, 128u, 128u },/* Dim White 50% (Default) */
    { 0u, 0u, 0u },      /* OFF / Black */
    { 40u, 0u, 0u },     /* Dim Red */
    { 0u, 0u, 40u },     /* Dim Blue */
    { 0u, 40u, 0u },     /* Dim Green */
    { 40u, 20u, 0u },    /* Dim Amber */
    { 30u, 30u, 30u },   /* Low Dim White */
    { 0u, 30u, 30u },    /* Dim Cyan */
    { 25u, 0u, 40u }     /* Dim Purple */
};

#define OFF_PALETTE_SIZE (sizeof(s_off_palette) / sizeof(s_off_palette[0]))

static bool colors_match(rgb_color_t a, rgb_color_t b)
{
    return (a.r == b.r) && (a.g == b.g) && (a.b == b.b);
}

void backlight_manager_toggle_enabled(void)
{
    bool new_state = true;
    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        s_config.enabled = !s_config.enabled;
        new_state = s_config.enabled;
        persist_config();
        xSemaphoreGive(g_config_mutex);
    }

    LOG_INFO("Backlight: Toggled to %s", new_state ? "ENABLED" : "DISABLED");

    rgb_update_t update = {
        .type     = RGB_UPDATE_BACKLIGHT_ONOFF,
        .param    = new_state ? 1u : 0u,
        .led_mask = 0u
    };
    xQueueSend(g_rgb_update_queue, &update, 0);
}

void backlight_manager_brightness_inc(void)
{
    uint8_t cur = 100u;
    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        if (s_config.brightness <= 90u)
        {
            s_config.brightness = (uint8_t) (s_config.brightness + 10u);
        }
        else
        {
            s_config.brightness = 100u;
        }
        cur = s_config.brightness;
        persist_config();
        xSemaphoreGive(g_config_mutex);
    }

    LOG_INFO("Backlight: Brightness Increased to %u%%", cur);

    rgb_update_t update = {
        .type     = RGB_UPDATE_BRIGHTNESS,
        .param    = cur,
        .led_mask = 0u
    };
    xQueueSend(g_rgb_update_queue, &update, 0);
}

void backlight_manager_brightness_dec(void)
{
    uint8_t cur = 10u;
    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        if (s_config.brightness >= 20u)
        {
            s_config.brightness = (uint8_t) (s_config.brightness - 10u);
        }
        else
        {
            s_config.brightness = 10u; /* Minimum visible floor */
        }
        cur = s_config.brightness;
        persist_config();
        xSemaphoreGive(g_config_mutex);
    }

    LOG_INFO("Backlight: Brightness Decreased to %u%%", cur);

    rgb_update_t update = {
        .type     = RGB_UPDATE_BRIGHTNESS,
        .param    = cur,
        .led_mask = 0u
    };
    xQueueSend(g_rgb_update_queue, &update, 0);
}

void backlight_manager_cycle_on_color(void)
{
    rgb_color_t new_color = s_on_palette[0];
    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        uint8_t next_idx = 0u;
        for (uint8_t i = 0u; i < ON_PALETTE_SIZE; i++)
        {
            if (colors_match(s_config.on_color, s_on_palette[i]))
            {
                next_idx = (uint8_t) ((i + 1u) % ON_PALETTE_SIZE);
                break;
            }
        }
        s_config.on_color = s_on_palette[next_idx];
        new_color = s_config.on_color;
        persist_config();
        xSemaphoreGive(g_config_mutex);
    }

    LOG_INFO("Backlight: Cycled ON Color to RGB(%u, %u, %u)", new_color.r, new_color.g, new_color.b);
    backlight_manager_refresh_all();
}

void backlight_manager_cycle_off_color(void)
{
    rgb_color_t new_color = s_off_palette[0];
    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        uint8_t next_idx = 0u;
        for (uint8_t i = 0u; i < OFF_PALETTE_SIZE; i++)
        {
            if (colors_match(s_config.off_color, s_off_palette[i]))
            {
                next_idx = (uint8_t) ((i + 1u) % OFF_PALETTE_SIZE);
                break;
            }
        }
        s_config.off_color = s_off_palette[next_idx];
        new_color = s_config.off_color;
        persist_config();
        xSemaphoreGive(g_config_mutex);
    }

    LOG_INFO("Backlight: Cycled OFF Color to RGB(%u, %u, %u)", new_color.r, new_color.g, new_color.b);
    backlight_manager_refresh_all();
}


