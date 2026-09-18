/*
 * tuya_dp_handlers.c
 *
 * Implements Tuya DP processing and cloud status synchronization.
 */

#include "bsp_api.h"
#include "app/tuya_dp_handlers.h"
#include "middleware/tuya_mcu_sdk/wifi.h"
#include "middleware/tuya_mcu_sdk/protocol.h"
#include "middleware/tuya_mcu_sdk/mcu_api.h"

#include "app/light_manager.h"
#include "app/backlight_manager.h"
#include "app/countdown_manager.h"
#include "app/factory_reset_manager.h"
#include "app/master_switch_manager.h"
#include "app/state_persistence_manager.h"
#include "app/app_common.h"
#include "app/app_log.h"

#include <string.h>

static bool    s_color_target = true;  /* true = ON color, false = OFF color */
static uint8_t s_relay_status = 2u;    /* 0=OFF, 1=ON, 2=MEMORY (Restore Last State) */

bool tuya_dp_handle_switch(uint8_t switch_id, bool on)
{
    if ((switch_id >= 1u) && (switch_id <= LIGHT_SWITCH_COUNT))
    {
        LOG_INFO("Tuya DP: Switch %u set to %s", switch_id, on ? "ON" : "OFF");
        light_manager_set_switch(switch_id, on);
        backlight_manager_notify_switch_state(switch_id, on);

        (void) mcu_dp_bool_update(DPID_SWITCH_1 + (switch_id - 1u), on);
        (void) mcu_dp_bool_update(DPID_SWITCH_ALL, (light_manager_get_mask() == 0x3Fu));
        if (switch_id >= 2u)
        {
            master_switch_manager_notify_manual_change();
        }
        return true;
    }

    return false;
}

bool tuya_dp_handle_countdown(uint8_t switch_id, uint32_t seconds)
{
    if ((switch_id >= 1u) && (switch_id <= LIGHT_SWITCH_COUNT))
    {
        LOG_INFO("Tuya DP: Countdown SW%u set to %lu s", switch_id, (unsigned long) seconds);
        countdown_manager_set_light(switch_id, seconds);
        (void) mcu_dp_value_update(DPID_COUNTDOWN_1 + (switch_id - 1u), seconds);
        return true;
    }

    return false;
}

bool tuya_dp_handle_switch_all(bool on)
{
    LOG_INFO("Tuya DP: Master Switch ALL set to %s", on ? "ON" : "OFF");
    if (on)
    {
        light_manager_all_on();
    }
    else
    {
        light_manager_all_off();
    }

    backlight_manager_refresh_all();

    (void) mcu_dp_bool_update(DPID_SWITCH_ALL, on);
    (void) mcu_dp_bool_update(DPID_SWITCH_1, on);
    (void) mcu_dp_bool_update(DPID_SWITCH_2, on);
    (void) mcu_dp_bool_update(DPID_SWITCH_3, on);
    (void) mcu_dp_bool_update(DPID_SWITCH_4, on);
    (void) mcu_dp_bool_update(DPID_SWITCH_5, on);
    (void) mcu_dp_bool_update(DPID_SWITCH_6, on);

    return true;
}

bool tuya_dp_handle_relay_status(uint8_t status)
{
    LOG_INFO("Tuya DP: Power Recovery (Relay Status) set to %u (0=OFF, 1=ON, 2=MEMORY)", status);
    if (status <= 2u)
    {
        s_relay_status = status;
    }

    (void) mcu_dp_enum_update(DPID_RELAY_STATUS, s_relay_status);
    return true;
}

bool tuya_dp_handle_backlight_switch(bool on)
{
    LOG_INFO("Tuya DP: Backlight Switch set to %s", on ? "ON" : "OFF");
    backlight_manager_set_enabled(on);
    (void) mcu_dp_bool_update(DPID_BACKLIGHT_SWITCH, on);
    return true;
}

bool tuya_dp_handle_set_color(bool target)
{
    LOG_INFO("Tuya DP: Set Color target set to %s color", target ? "ON" : "OFF");
    s_color_target = target;
    (void) mcu_dp_bool_update(DPID_SET_COLOR, target);

    rgb_color_t color;
    if (target)
    {
        backlight_manager_get_on_color(&color);
    }
    else
    {
        backlight_manager_get_off_color(&color);
    }

    uint32_t packed = ((uint32_t) color.r << 16) | ((uint32_t) color.g << 8) | (uint32_t) color.b;
    (void) mcu_dp_value_update(DPID_COLOR_VALUES, packed);

    return true;
}

bool tuya_dp_handle_color_values(uint32_t color_val)
{
    rgb_color_t color;
    color.r = (uint8_t) ((color_val >> 16) & 0xFFu);
    color.g = (uint8_t) ((color_val >> 8) & 0xFFu);
    color.b = (uint8_t) (color_val & 0xFFu);

    LOG_INFO("Tuya DP: Color Value set to RGB(0x%02X, 0x%02X, 0x%02X) for %s state",
             color.r, color.g, color.b, s_color_target ? "ON" : "OFF");

    if (s_color_target)
    {
        backlight_manager_set_on_color(&color);
    }
    else
    {
        backlight_manager_set_off_color(&color);
    }

    (void) mcu_dp_value_update(DPID_COLOR_VALUES, color_val);
    return true;
}

bool tuya_dp_handle_bright_per(uint32_t brightness)
{
    if (brightness > 100u)
    {
        brightness = 100u;
    }

    LOG_INFO("Tuya DP: Brightness set to %lu%%", (unsigned long) brightness);
    backlight_manager_set_brightness((uint8_t) brightness);
    (void) mcu_dp_value_update(DPID_BRIGHT_PER, brightness);
    return true;
}

bool tuya_dp_handle_child_lock(bool locked)
{
    LOG_INFO("Tuya DP: Child Lock set to %s", locked ? "LOCKED" : "UNLOCKED");
    if (locked)
    {
        xEventGroupSetBits(g_system_state_event_group, SYS_BIT_CHILD_LOCK);
    }
    else
    {
        xEventGroupClearBits(g_system_state_event_group, SYS_BIT_CHILD_LOCK);
    }

    (void) mcu_dp_bool_update(DPID_CHILD_LOCK, locked);
    return true;
}

bool tuya_dp_handle_factory_reset(bool reset)
{
    LOG_INFO("Tuya DP: Factory Reset requested (val=%u)", reset ? 1u : 0u);
    if (reset)
    {
        factory_reset_manager_execute();
    }

    (void) mcu_dp_bool_update(DPID_FACTORY_RESET, false);
    return true;
}

void tuya_dp_sync_all(void)
{
    LOG_INFO("Tuya DP: Reporting full system status sync (all_data_update)...");

    /* Light switches 1..6 */
    (void) mcu_dp_bool_update(DPID_SWITCH_1, light_manager_get_switch(1u));
    (void) mcu_dp_bool_update(DPID_SWITCH_2, light_manager_get_switch(2u));
    (void) mcu_dp_bool_update(DPID_SWITCH_3, light_manager_get_switch(3u));
    (void) mcu_dp_bool_update(DPID_SWITCH_4, light_manager_get_switch(4u));
    (void) mcu_dp_bool_update(DPID_SWITCH_5, light_manager_get_switch(5u));
    (void) mcu_dp_bool_update(DPID_SWITCH_6, light_manager_get_switch(6u));

    /* Countdown timers 1..6 */
    (void) mcu_dp_value_update(DPID_COUNTDOWN_1, countdown_manager_get_light(1u));
    (void) mcu_dp_value_update(DPID_COUNTDOWN_2, countdown_manager_get_light(2u));
    (void) mcu_dp_value_update(DPID_COUNTDOWN_3, countdown_manager_get_light(3u));
    (void) mcu_dp_value_update(DPID_COUNTDOWN_4, countdown_manager_get_light(4u));
    (void) mcu_dp_value_update(DPID_COUNTDOWN_5, countdown_manager_get_light(5u));
    (void) mcu_dp_value_update(DPID_COUNTDOWN_6, countdown_manager_get_light(6u));

    /* Master Switch */
    (void) mcu_dp_bool_update(DPID_SWITCH_ALL, (light_manager_get_mask() == 0x3Fu));

    /* Power Recovery Mode */
    (void) mcu_dp_enum_update(DPID_RELAY_STATUS, s_relay_status);

    /* Backlight Switch & Brightness */
    (void) mcu_dp_bool_update(DPID_BACKLIGHT_SWITCH, backlight_manager_get_enabled());
    (void) mcu_dp_value_update(DPID_BRIGHT_PER, backlight_manager_get_brightness());

    /* Color configuration */
    (void) mcu_dp_bool_update(DPID_SET_COLOR, s_color_target);
    rgb_color_t color;
    if (s_color_target)
    {
        backlight_manager_get_on_color(&color);
    }
    else
    {
        backlight_manager_get_off_color(&color);
    }
    uint32_t packed = ((uint32_t) color.r << 16) | ((uint32_t) color.g << 8) | (uint32_t) color.b;
    (void) mcu_dp_value_update(DPID_COLOR_VALUES, packed);

    /* Child Lock */
    EventBits_t sys_bits = xEventGroupGetBits(g_system_state_event_group);
    (void) mcu_dp_bool_update(DPID_CHILD_LOCK, ((sys_bits & SYS_BIT_CHILD_LOCK) != 0u));

    /* Factory Reset DP */
    (void) mcu_dp_bool_update(DPID_FACTORY_RESET, 0u);
}

void tuya_dp_notify_switch_changed(uint8_t switch_id, bool on)
{
    if ((switch_id >= 1u) && (switch_id <= LIGHT_SWITCH_COUNT))
    {
        tuya_dp_cmd_t cmd;
        cmd.dpid   = (uint8_t)(DPID_SWITCH_1 + (switch_id - 1u));
        cmd.length = 1u;
        cmd.value[0] = on ? 1u : 0u;
        (void) xQueueSend(g_tuya_dp_queue, &cmd, 0u);

        /* Also queue Master Switch report */
        cmd.dpid   = DPID_SWITCH_ALL;
        cmd.length = 1u;
        cmd.value[0] = (light_manager_get_mask() == 0x3Fu) ? 1u : 0u;
        (void) xQueueSend(g_tuya_dp_queue, &cmd, 0u);
    }
}

void tuya_dp_notify_countdown_changed(uint8_t switch_id, uint32_t seconds)
{
    if ((switch_id >= 1u) && (switch_id <= LIGHT_SWITCH_COUNT))
    {
        tuya_dp_cmd_t cmd;
        cmd.dpid   = (uint8_t)(DPID_COUNTDOWN_1 + (switch_id - 1u));
        cmd.length = 4u;
        cmd.value[0] = (uint8_t)((seconds >> 24) & 0xFFu);
        cmd.value[1] = (uint8_t)((seconds >> 16) & 0xFFu);
        cmd.value[2] = (uint8_t)((seconds >> 8) & 0xFFu);
        cmd.value[3] = (uint8_t)(seconds & 0xFFu);
        (void) xQueueSend(g_tuya_dp_queue, &cmd, 0u);
    }
}

void tuya_dp_notify_switch_all_changed(bool on)
{
    tuya_dp_cmd_t cmd;
    cmd.dpid   = DPID_SWITCH_ALL;
    cmd.length = 1u;
    cmd.value[0] = on ? 1u : 0u;
    (void) xQueueSend(g_tuya_dp_queue, &cmd, 0u);
}

void tuya_dp_notify_child_lock_changed(bool locked)
{
    tuya_dp_cmd_t cmd;
    cmd.dpid   = DPID_CHILD_LOCK;
    cmd.length = 1u;
    cmd.value[0] = locked ? 1u : 0u;
    (void) xQueueSend(g_tuya_dp_queue, &cmd, 0u);
}

void tuya_dp_notify_backlight_changed(bool on)
{
    tuya_dp_cmd_t cmd;
    cmd.dpid   = DPID_BACKLIGHT_SWITCH;
    cmd.length = 1u;
    cmd.value[0] = on ? 1u : 0u;
    (void) xQueueSend(g_tuya_dp_queue, &cmd, 0u);
}

void tuya_dp_notify_brightness_changed(uint8_t brightness)
{
    tuya_dp_cmd_t cmd;
    cmd.dpid   = DPID_BRIGHT_PER;
    cmd.length = 4u;
    cmd.value[0] = 0u;
    cmd.value[1] = 0u;
    cmd.value[2] = 0u;
    cmd.value[3] = brightness;
    (void) xQueueSend(g_tuya_dp_queue, &cmd, 0u);
}

void tuya_dp_notify_color_changed(rgb_color_t color)
{
    tuya_dp_cmd_t cmd;
    cmd.dpid   = DPID_COLOR_VALUES;
    cmd.length = 4u;
    cmd.value[0] = 0u;
    cmd.value[1] = color.r;
    cmd.value[2] = color.g;
    cmd.value[3] = color.b;
    (void) xQueueSend(g_tuya_dp_queue, &cmd, 0u);
}

void tuya_dp_process_queue(void)
{
    tuya_dp_cmd_t cmd;

    while (xQueueReceive(g_tuya_dp_queue, &cmd, 0u) == pdTRUE)
    {
        switch (cmd.dpid)
        {
            case DPID_SWITCH_1:
            case DPID_SWITCH_2:
            case DPID_SWITCH_3:
            case DPID_SWITCH_4:
            case DPID_SWITCH_5:
            case DPID_SWITCH_6:
            case DPID_SWITCH_ALL:
            case DPID_BACKLIGHT_SWITCH:
            case DPID_SET_COLOR:
            case DPID_CHILD_LOCK:
            case DPID_FACTORY_RESET:
                (void) mcu_dp_bool_update(cmd.dpid, cmd.value[0]);
                break;

            case DPID_COUNTDOWN_1:
            case DPID_COUNTDOWN_2:
            case DPID_COUNTDOWN_3:
            case DPID_COUNTDOWN_4:
            case DPID_COUNTDOWN_5:
            case DPID_COUNTDOWN_6:
            case DPID_COLOR_VALUES:
            case DPID_BRIGHT_PER:
            {
                uint32_t val = ((uint32_t) cmd.value[0] << 24) |
                               ((uint32_t) cmd.value[1] << 16) |
                               ((uint32_t) cmd.value[2] << 8)  |
                               (uint32_t) cmd.value[3];
                (void) mcu_dp_value_update(cmd.dpid, val);
                break;
            }

            case DPID_RELAY_STATUS:
                (void) mcu_dp_enum_update(cmd.dpid, cmd.value[0]);
                break;

            default:
                break;
        }
    }
}

