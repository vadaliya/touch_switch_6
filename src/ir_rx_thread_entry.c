/*
 * ir_rx_thread_entry.c
 *
 * *** SIMPLIFIED: hardware capture removes the need for the old
 * priority-1 + tight taskYIELD() workaround ***. That existed because
 * software polling needed to be scheduled as often as possible to
 * avoid missing edges -- with GPT5 hardware capture now doing the
 * actual edge timestamping (immune to touch_thread preemption),
 * ir_driver_process() blocks properly instead of spinning, so this
 * thread can run at its originally-intended priority like any other
 * thread in the system. Restore whatever priority you configured for
 * this thread in the FSP Configurator's Stacks tab (the vTaskPrioritySet
 * override below is removed).
 */

#include "app/app_common.h"
#include "app/backlight_manager.h"
#include "app/ir_learning_manager.h"
#include "app/light_manager.h"
#include "app/master_switch_manager.h"
#include "app/scene_manager.h"
#include "drivers/ir_driver.h"
#include "app/tuya_dp_handlers.h"

#define IR_KEY_RELEASE_TIMEOUT_MS  (350u)

static uint16_t s_last_handled_code    = 0u;
static bool     s_last_handled_toggle  = false;
static uint32_t s_last_handled_time_ms = 0u;

extern void ir_rx_thread_entry(void *pvParameters);

static void handle_ir_event(const ir_message_t *msg)
{
    if (msg == NULL)
    {
        return;
    }

    uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    bool is_repeat = false;

    uint16_t cmd_code = ir_driver_strip_toggle(msg->raw_code);

    if ((cmd_code == ir_driver_strip_toggle(s_last_handled_code)) &&
        (msg->toggle == s_last_handled_toggle) &&
        ((now_ms - s_last_handled_time_ms) < IR_KEY_RELEASE_TIMEOUT_MS))
    {
        is_repeat = true;
    }

    s_last_handled_code    = msg->raw_code;
    s_last_handled_toggle  = msg->toggle;
    s_last_handled_time_ms = now_ms;

    if (is_repeat)
    {
        /* NOTE: silently suppressed -- this is the exact case that
         * previously looked identical to a genuine decode failure in
         * the log, since nothing logged here. Still true in this
         * version; flagged for whenever the logging pass happens. */
        return;
    }

    if (ir_learning_manager_is_learning())
    {
        uint8_t scene_idx = 0u;
        bool is_scene_btn = ir_driver_is_scene_button(msg->address, msg->command, &scene_idx);
        bool is_fixed_btn = ir_driver_is_fixed_control_button(msg->address, msg->command);

        if (ir_learning_manager_get_state() == IR_LEARN_STATE_SCENE)
        {
            /* Only dedicated Scene buttons are allowed for IR Scene Learning */
            if (!is_scene_btn)
            {
                LOG_WARNING("IR Scene Learn: Button (Addr=%u Cmd=%u) is not a dedicated Scene button! Rejected.",
                            msg->address, msg->command);
                return;
            }

            LOG_INFO("IR Scene Learn: Received dedicated Scene %u button (Addr=%u Cmd=%u)",
                     scene_idx + 1u, msg->address, msg->command);
            (void) ir_learning_manager_handle_scene_ir_event(scene_idx, msg->raw_code);
            return;
        }
        else
        {
            /* Switch Learning Mode */
            if (is_scene_btn)
            {
                LOG_WARNING("IR Learn: Dedicated Scene button (Addr=%u Cmd=%u) cannot be assigned to switch functions! Rejected.",
                            msg->address, msg->command);
                return;
            }

            if (is_fixed_btn)
            {
                LOG_WARNING("IR Learn: Fixed remote command (Addr=%u Cmd=%u) cannot be learned for switches! Rejected.",
                            msg->address, msg->command);
                return;
            }

            LOG_INFO("IR Learning: Storing code 0x%04X (Addr=%u Cmd=%u)", msg->raw_code, msg->address, msg->command);
            ir_learning_manager_handle_ir_event(msg->raw_code);
            return;
        }
    }

    if (cmd_code == 0u)
    {
        return;
    }

    /* Fixed IR Remote Commands */
    if (cmd_code == IR_CODE_FIXED_ALL_ON)
    {
        LOG_INFO("IR Match: ALL ON (Code=0x%04X)", msg->raw_code);
        light_manager_all_on();
        backlight_manager_refresh_all();
        tuya_dp_notify_switch_all_changed(true);
        for (uint8_t s = 1u; s <= LIGHT_SWITCH_COUNT; s++)
        {
            tuya_dp_notify_switch_changed(s, true);
        }
        return;
    }

    if (cmd_code == IR_CODE_FIXED_ALL_OFF)
    {
        LOG_INFO("IR Match: ALL OFF (Code=0x%04X)", msg->raw_code);
        light_manager_all_off();
        backlight_manager_refresh_all();
        tuya_dp_notify_switch_all_changed(false);
        for (uint8_t s = 1u; s <= LIGHT_SWITCH_COUNT; s++)
        {
            tuya_dp_notify_switch_changed(s, false);
        }
        return;
    }

    /* Fixed Remote R2 Commands (RC5 Address 15) */
    if (cmd_code == IR_CODE_FIXED_BACKLIGHT_TOGGLE)
    {
        LOG_INFO("IR Match: Backlight ON/OFF Toggle (Code=0x%04X)", msg->raw_code);
        backlight_manager_toggle_enabled();
        tuya_dp_notify_backlight_changed(backlight_manager_get_enabled());
        return;
    }

    if (cmd_code == IR_CODE_FIXED_BACKLIGHT_ON_COLOR)
    {
        LOG_INFO("IR Match: Backlight ON Color Change (Code=0x%04X)", msg->raw_code);
        backlight_manager_cycle_on_color();
        rgb_color_t color;
        backlight_manager_get_on_color(&color);
        tuya_dp_notify_color_changed(color);
        return;
    }

    if (cmd_code == IR_CODE_FIXED_BACKLIGHT_OFF_COLOR)
    {
        LOG_INFO("IR Match: Backlight OFF Color Change (Code=0x%04X)", msg->raw_code);
        backlight_manager_cycle_off_color();
        rgb_color_t color;
        backlight_manager_get_off_color(&color);
        tuya_dp_notify_color_changed(color);
        return;
    }

    if (cmd_code == IR_CODE_FIXED_BACKLIGHT_BRIGHTNESS_INC)
    {
        LOG_INFO("IR Match: Backlight Brightness Increment (Code=0x%04X)", msg->raw_code);
        backlight_manager_brightness_inc();
        tuya_dp_notify_brightness_changed(backlight_manager_get_brightness());
        return;
    }

    if (cmd_code == IR_CODE_FIXED_BACKLIGHT_BRIGHTNESS_DEC)
    {
        LOG_INFO("IR Match: Backlight Brightness Decrement (Code=0x%04X)", msg->raw_code);
        backlight_manager_brightness_dec();
        tuya_dp_notify_brightness_changed(backlight_manager_get_brightness());
        return;
    }

    if (cmd_code == IR_CODE_FIXED_CHILD_LOCK_TOGGLE)
    {
        EventBits_t sys_bits = xEventGroupGetBits(g_system_state_event_group);
        bool is_locked = ((sys_bits & SYS_BIT_CHILD_LOCK) != 0u);
        bool new_locked = !is_locked;
        if (new_locked)
        {
            xEventGroupSetBits(g_system_state_event_group, SYS_BIT_CHILD_LOCK);
            LOG_INFO("IR Match: Child Lock ENABLED (Code=0x%04X)", msg->raw_code);
        }
        else
        {
            xEventGroupClearBits(g_system_state_event_group, SYS_BIT_CHILD_LOCK);
            LOG_INFO("IR Match: Child Lock DISABLED (Code=0x%04X)", msg->raw_code);
        }
        tuya_dp_notify_child_lock_changed(new_locked);
        return;
    }

    /* Learned switch codes */
    for (uint8_t i = 1u; i <= LIGHT_SWITCH_COUNT; i++)
    {
        uint16_t sw_code = ir_driver_strip_toggle(ir_learning_manager_get_switch_code(i));
        if ((sw_code != 0u) && (sw_code == cmd_code))
        {
            LOG_INFO("IR Match: Switch %u (Code=0x%04X)", i, msg->raw_code);
            if ((i == 1u) && master_switch_manager_is_enabled())
            {
                master_switch_manager_press();
            }
            else
            {
                light_manager_toggle_switch(i);
                bool new_state = light_manager_get_switch(i);
                backlight_manager_notify_switch_state(i, new_state);
                tuya_dp_notify_switch_changed(i, new_state);
                master_switch_manager_notify_manual_change();
            }
            return;
        }
    }

    /* Learned scene codes */
    if (scene_manager_execute_by_code(cmd_code))
    {
        for (uint8_t s = 1u; s <= LIGHT_SWITCH_COUNT; s++)
        {
            tuya_dp_notify_switch_changed(s, light_manager_get_switch(s));
        }
        master_switch_manager_notify_manual_change();
        return;
    }

    LOG_INFO("IR Received (Unmapped): Code=0x%04X Addr=%u Cmd=%u", msg->raw_code, msg->address, msg->command);
}

void ir_rx_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    LOG_INFO("IR RX Thread started (P103, GPT5 hardware capture)");

    ir_driver_init();
    ir_learning_manager_init();
    scene_manager_init();

    TickType_t last_tick_check = xTaskGetTickCount();

    for (;;)
    {
        ir_message_t msg;
        if (ir_driver_process(&msg))
        {
            handle_ir_event(&msg);
        }
        /* No taskYIELD()/vTaskDelay needed -- ir_driver_process()
         * already blocks internally (up to IR_IDLE_TIMEOUT_MS) when
         * there's nothing to drain, so this loop self-paces. */

        /* Advance IR learning mode timeout using REAL elapsed time --
         * the old tight-loop version assumed ~1ms/iteration, which no
         * longer holds now that ir_driver_process() can block for
         * varying durations. */
        TickType_t now_tick = xTaskGetTickCount();
        uint32_t elapsed_ms = (uint32_t) ((now_tick - last_tick_check) * portTICK_PERIOD_MS);
        last_tick_check = now_tick;
        ir_learning_manager_tick(elapsed_ms);

        xEventGroupSetBits(g_heartbeat_event_group, HEARTBEAT_BIT_IR_RX);
    }
}
