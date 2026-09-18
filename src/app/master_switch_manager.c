/*
 * master_switch_manager.c
 *
 * Implements Switch-1 Master Switch functionality for 6-switch module.
 * When enabled, Switch-1 acts as Master Switch controlling Switches 2-6.
 */

#include "app/master_switch_manager.h"
#include "app/state_persistence_manager.h"
#include "app/light_manager.h"
#include "app/backlight_manager.h"
#include "app/app_common.h"
#include "app/app_log.h"
#include "drivers/eeprom_driver.h"
#include "drivers/rgb_led_driver.h"
#include "app/tuya_dp_handlers.h"

#include <string.h>

typedef struct
{
    uint8_t magic;     /* 0xD4 */
    uint8_t is_master; /* 1 = master, 0 = regular */
    uint8_t reserved;  /* 0x00 */
    uint8_t checksum;  /* magic ^ is_master ^ reserved */
} master_switch_record_t;

static bool        s_is_master_switch  = false;
static bool        s_loaded            = false;
static bool        s_has_snapshot      = false;
static uint8_t     s_saved_switch_mask = 0u;

static uint8_t calc_checksum(const master_switch_record_t *rec)
{
    return (uint8_t) (rec->magic ^ rec->is_master ^ rec->reserved);
}

static bool persist_master_switch_state(bool is_master)
{
    /* Read existing IR codes & scenes from 0x000 */
    uint8_t ir_buf[38];
    bool has_ir = eeprom_read(0u, ir_buf, sizeof(ir_buf), 100u);

    /* Read existing Backlight config from 0x100 */
    uint8_t bl_buf[16];
    bool has_bl = eeprom_read(BACKLIGHT_EEPROM_OFFSET, bl_buf, sizeof(bl_buf), 100u);

    /* Read existing Power Recovery config from 0x110 */
    uint8_t pr_buf[4];
    bool has_pr = eeprom_read(POWER_RECOVERY_EEPROM_OFFSET, pr_buf, sizeof(pr_buf), 100u);

    master_switch_record_t rec;
    rec.magic     = MASTER_SWITCH_MAGIC;
    rec.is_master = is_master ? 1u : 0u;
    rec.reserved  = 0u;
    rec.checksum  = calc_checksum(&rec);

    bool ok = false;
    if (eeprom_erase_blocking(0u, 1u, 2000u))
    {
        if (has_ir)
        {
            (void) eeprom_write_blocking(0u, ir_buf, sizeof(ir_buf), 1000u);
        }
        if (has_bl)
        {
            (void) eeprom_write_blocking(BACKLIGHT_EEPROM_OFFSET, bl_buf, sizeof(bl_buf), 1000u);
        }
        if (has_pr)
        {
            (void) eeprom_write_blocking(POWER_RECOVERY_EEPROM_OFFSET, pr_buf, sizeof(pr_buf), 1000u);
        }
        ok = eeprom_write_blocking(MASTER_SWITCH_EEPROM_OFFSET, (const uint8_t *) &rec, sizeof(rec), 1000u);
    }

    if (ok)
    {
        LOG_INFO("Master Switch: Config Saved to Flash: is_master=%u", is_master ? 1u : 0u);
    }
    else
    {
        LOG_ERROR("Master Switch: Failed to save config to Flash!");
    }

    return ok;
}

void master_switch_manager_init(void)
{
    master_switch_record_t rec;
    if (eeprom_read(MASTER_SWITCH_EEPROM_OFFSET, (uint8_t *) &rec, sizeof(rec), 100u))
    {
        if ((rec.magic == MASTER_SWITCH_MAGIC) && (rec.checksum == calc_checksum(&rec)))
        {
            s_is_master_switch = (rec.is_master != 0u);
            s_loaded           = true;
            LOG_INFO("Master Switch: Config Loaded from Flash: is_master=%u", s_is_master_switch ? 1u : 0u);
            return;
        }
    }

    /* Unprogrammed or default */
    s_is_master_switch = false;
    s_loaded           = true;
    LOG_INFO("Master Switch: Unconfigured in Flash, defaulting to regular switch.");
}

bool master_switch_manager_is_enabled(void)
{
    if (!s_loaded)
    {
        master_switch_manager_init();
    }
    return s_is_master_switch;
}

void master_switch_manager_toggle_mode(void)
{
    s_is_master_switch = !s_is_master_switch;
    s_has_snapshot     = false;

    persist_master_switch_state(s_is_master_switch);

    LOG_INFO("Master Switch: Switch-1 mode toggled -> %s",
             s_is_master_switch ? "MASTER SWITCH" : "REGULAR SWITCH");

    /* Visual confirmation: blink Switch 1 LED using ON color for 3 pulses (1.5s total) */
    rgb_color_t on_color;
    backlight_manager_get_on_color(&on_color);
    uint8_t sw1_mask = rgb_led_driver_switch_led_bit(1u);
    rgb_led_driver_start_blink_pulses(sw1_mask, on_color, 250u, 3u);

    /* Update Switch 1 backlight state to reflect current active status */
    if (s_is_master_switch)
    {
        bool any_on = false;
        for (uint8_t i = 2u; i <= 6u; i++)
        {
            if (light_manager_get_switch(i))
            {
                any_on = true;
                break;
            }
        }
        backlight_manager_notify_switch_state(1u, any_on);
    }
    else
    {
        backlight_manager_notify_switch_state(1u, light_manager_get_switch(1u));
    }
}

void master_switch_manager_press(void)
{
    if (!s_is_master_switch)
    {
        return;
    }

    bool any_on = false;
    for (uint8_t i = 2u; i <= 6u; i++)
    {
        if (light_manager_get_switch(i))
        {
            any_on = true;
            break;
        }
    }

    if (any_on)
    {
        /* Snapshot the currently ON outputs for SW2-SW6 */
        s_saved_switch_mask = 0u;
        for (uint8_t i = 2u; i <= 6u; i++)
        {
            if (light_manager_get_switch(i))
            {
                s_saved_switch_mask |= (uint8_t) (1u << i);
            }
        }
        s_has_snapshot = true;

        LOG_INFO("Master Switch PRESS: Captured ON snapshot (mask=0x%02X), turning all OFF", s_saved_switch_mask);

        /* Turn all controlled outputs (SW2-SW6) OFF */
        for (uint8_t i = 2u; i <= 6u; i++)
        {
            if (light_manager_get_switch(i))
            {
                light_manager_set_switch(i, false);
                backlight_manager_notify_switch_state(i, false);
                tuya_dp_notify_switch_changed(i, false);
            }
        }

        /* Switch 1 LED reflects that all controlled switches are now OFF */
        backlight_manager_notify_switch_state(1u, false);
    }
    else
    {
        /* All controlled outputs are currently OFF */
        if (s_has_snapshot && (s_saved_switch_mask != 0u))
        {
            LOG_INFO("Master Switch PRESS: Restoring saved snapshot (mask=0x%02X)", s_saved_switch_mask);

            for (uint8_t i = 2u; i <= 6u; i++)
            {
                if ((s_saved_switch_mask & (uint8_t) (1u << i)) != 0u)
                {
                    light_manager_set_switch(i, true);
                    backlight_manager_notify_switch_state(i, true);
                    tuya_dp_notify_switch_changed(i, true);
                }
            }

            /* Switch 1 LED reflects that master outputs are now ON */
            backlight_manager_notify_switch_state(1u, true);
        }
        else
        {
            LOG_INFO("Master Switch PRESS: All switches OFF and no stored snapshot, doing nothing.");
        }
    }
}

void master_switch_manager_notify_manual_change(void)
{
    if (!s_is_master_switch)
    {
        return;
    }

    bool any_on = false;
    for (uint8_t i = 2u; i <= 6u; i++)
    {
        if (light_manager_get_switch(i))
        {
            any_on = true;
            break;
        }
    }

    if (!any_on)
    {
        /* User turned off all switches manually -- clear active master snapshot */
        s_has_snapshot = false;
        backlight_manager_notify_switch_state(1u, false);
    }
    else
    {
        backlight_manager_notify_switch_state(1u, true);
    }
}

void master_switch_manager_reset(void)
{
    s_is_master_switch  = false;
    s_has_snapshot      = false;
    s_saved_switch_mask = 0u;

    persist_master_switch_state(false);
    LOG_INFO("Master Switch: Reset to default (regular switch).");
}
