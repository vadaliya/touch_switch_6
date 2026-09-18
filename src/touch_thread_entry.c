/*
 * touch_thread_entry.c
 *
 * FSP-generated once for the "Touch Thread" (symbol: touch_thread).
 *
 * Periodically triggers an rm_touch scan (Touch middleware, calibrated
 * via QE for Capacitive Touch) covering the 6 touch buttons S1..S6,
 * and posts touch_event_t items to g_touch_event_queue
 * for the application-layer managers to consume. See touch_driver.c
 * for the long-press-timing logic -- debounce and touch thresholding
 * themselves are handled by rm_touch.
 *
 * LED updates flow cleanly through backlight_manager into
 * g_rgb_update_queue to be rendered by rgb_thread.
 */

#include "app/app_common.h"
#include "app/backlight_manager.h"
#include "app/ir_learning_manager.h"
#include "app/light_manager.h"
#include "app/master_switch_manager.h"
#include "app/scene_manager.h"
#include "app/state_persistence_manager.h"
#include "app/wifi_config_manager.h"
#include "drivers/touch_driver.h"
#include "app/tuya_dp_handlers.h"

#include "qe_touch_config.h"

#ifndef QE_TUNNING_TESTING
#define QE_TUNNING_TESTING 0
#endif


#if QE_TUNNING_TESTING
uint64_t button_status_temp;
uint16_t slider_position_temp[TOUCH_CFG_NUM_SLIDERS];
fsp_err_t err_temp;
#endif

#define TOUCH_SCAN_PERIOD_MS   (20u)   /* trigger rate for hal_touch_scan_and_wait();
                                          keep in sync with any related timing elsewhere */

extern void touch_thread_entry(void *pvParameters);

static void handle_touch_event(const touch_event_t *evt)
{
    if (evt == NULL)
    {
        return;
    }

    EventBits_t sys_bits = xEventGroupGetBits(g_system_state_event_group);
    bool child_locked = ((sys_bits & SYS_BIT_CHILD_LOCK) != 0u);

    if (child_locked)
    {
        /* Under Child Lock: Local short presses for lights are blocked.
         * Advanced actions (5s IR learn, 10s Wi-Fi pairing) and remote commands remain functional. */
        if ((evt->type >= TOUCH_EVT_S1_SHORT_PRESS) && (evt->type <= TOUCH_EVT_S6_SHORT_PRESS))
        {
            LOG_WARNING("Touch event %d ignored: Child Lock active", evt->type);
            return;
        }
    }

    switch (evt->type)
    {
        case TOUCH_EVT_S1_SHORT_PRESS:
            if (master_switch_manager_is_enabled())
            {
                master_switch_manager_press();
            }
            else
            {
                light_manager_toggle_switch(1u);
                bool new_state = light_manager_get_switch(1u);
                LOG_INFO("SW1 state=%u", new_state ? 1u : 0u);
                backlight_manager_notify_switch_state(1u, new_state);
                tuya_dp_notify_switch_changed(1u, new_state);
            }
            break;

        case TOUCH_EVT_S2_SHORT_PRESS:
        case TOUCH_EVT_S3_SHORT_PRESS:
        case TOUCH_EVT_S4_SHORT_PRESS:
        case TOUCH_EVT_S5_SHORT_PRESS:
        case TOUCH_EVT_S6_SHORT_PRESS:
            if ((evt->switch_id >= 2u) && (evt->switch_id <= LIGHT_SWITCH_COUNT))
            {
                light_manager_toggle_switch(evt->switch_id);
                bool new_state = light_manager_get_switch(evt->switch_id);
                LOG_INFO("SW%u state=%u", evt->switch_id, new_state ? 1u : 0u);
                backlight_manager_notify_switch_state(evt->switch_id, new_state);
                tuya_dp_notify_switch_changed(evt->switch_id, new_state);
                master_switch_manager_notify_manual_change();
            }
            break;

        case TOUCH_EVT_LONG_PRESS_5S:
        {
            uint8_t active_mask = light_manager_get_mask() & 0x3Fu;
            uint8_t active_count = scene_manager_count_outputs(active_mask);

            if (active_mask == 0u)
            {
                /* Pre-condition: All switches OFF -> Individual Switch IR Learning */
                if ((evt->switch_id >= 1u) && (evt->switch_id <= LIGHT_SWITCH_COUNT))
                {
                    LOG_INFO("IR Learn: Switch %u entered learning mode", evt->switch_id);
                    ir_learning_manager_handle_touch_long_press(evt->switch_id);
                }
            }
            else if ((active_count >= SCENE_MIN_OUTPUTS) && (active_count <= SCENE_MAX_OUTPUTS))
            {
                /* Pre-condition: Desired switches are ON (2..5 outputs).
                 * User presses and holds ANY ONE of the selected ON switches for 5s.
                 */
                bool pressed_is_on = false;
                if ((evt->switch_id >= 1u) && (evt->switch_id <= LIGHT_SWITCH_COUNT) &&
                    ((active_mask & (1u << (evt->switch_id - 1u))) != 0u))
                {
                    pressed_is_on = true;
                }

                if (pressed_is_on)
                {
                    LOG_INFO("IR Scene Learn: Entered Scene Learning for Mask=0x%02X (Held Switch %u)",
                             active_mask, evt->switch_id);
                    ir_learning_manager_start_scene_learning(active_mask);
                }
                else
                {
                    LOG_WARNING("IR Learn: Long-press ignored (held switch %u is OFF while other switches are ON)",
                                evt->switch_id);
                }
            }
            else if (active_count > SCENE_MAX_OUTPUTS)
            {
                LOG_WARNING("IR Scene Learn: Cannot create scene with all switches ON (Max %u outputs)",
                            SCENE_MAX_OUTPUTS);
            }
            else
            {
                LOG_WARNING("IR Learn: Long-press ignored (pre-conditions not met, active_mask=0x%02X)", active_mask);
            }
            break;
        }

        case TOUCH_EVT_LONG_PRESS_10S:
            if (evt->switch_id != 1u)
            {
                LOG_INFO("Touch Long-Press 10s (SW%u): Entering Wi-Fi SmartConfig pairing mode...", evt->switch_id);
                /* Cancel any IR learning mode triggered by the earlier 5s threshold of this press */
                ir_learning_manager_reset();
                wifi_config_manager_enter_smart_config();
            }
            break;

        case TOUCH_EVT_LONG_PRESS_15S:
            if (evt->switch_id == 1u)
            {
                LOG_INFO("Touch Long-Press 15s (SW%u): Toggling Switch-1 Master Switch mode...", evt->switch_id);
                ir_learning_manager_reset();
                master_switch_manager_toggle_mode();
            }
            break;

        default:
            break;
    }
}

void touch_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    touch_driver_init();
    LOG_INFO("Touch thread started");

    master_switch_manager_init();

    /* Restore power-on relay state from Data Flash */
    state_persistence_manager_restore_all();

    if (master_switch_manager_is_enabled())
    {
        bool any_on = light_manager_get_switch(2u) || light_manager_get_switch(3u) ||
                      light_manager_get_switch(4u) || light_manager_get_switch(5u) ||
                      light_manager_get_switch(6u);
        backlight_manager_notify_switch_state(1u, any_on);
    }

    /* Request full LED refresh to reflect restored state */
    backlight_manager_refresh_all();

    TickType_t last_wake_time = xTaskGetTickCount();

    for (;;)
    {
#if QE_TUNNING_TESTING
        /* for [CONFIG01] configuration */
        err_temp = RM_TOUCH_ScanStart(g_qe_touch_instance_config01.p_ctrl);
        if (FSP_SUCCESS != err_temp)
        {
            while (true) {}
        }
        while (0 == g_qe_touch_flag) {}
        g_qe_touch_flag = 0;

        err_temp = RM_TOUCH_DataGet(g_qe_touch_instance_config01.p_ctrl, &button_status_temp, slider_position_temp, NULL);
        if (FSP_SUCCESS == err_temp)
        {
            /* TODO: Add your own code here. */
        }

        /* FIXME: Since this is a temporary process, so re-create a waiting process yourself. */
        R_BSP_SoftwareDelay(TOUCH_SCAN_PERIOD_MS, BSP_DELAY_UNITS_MILLISECONDS);
#else
        touch_driver_scan();

        touch_event_t evt;
        while (xQueueReceive(g_touch_event_queue, &evt, 0) == pdTRUE)
        {
            LOG_INFO("Touch event type=%u switch=%u", (unsigned) evt.type, evt.switch_id);
            handle_touch_event(&evt);
        }

        /* EVENT GROUP: heartbeat check-in -- tells System thread this
        * thread completed a healthy iteration since the last check.
        */
        xEventGroupSetBits(g_heartbeat_event_group, HEARTBEAT_BIT_TOUCH);

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(TOUCH_SCAN_PERIOD_MS));
#endif // QE_TUNNING_TESTING
    }
}