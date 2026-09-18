/*
 * wifi_config_manager.c
 *
 * Implements Wi-Fi configuration triggers and LED pattern coordination.
 */

#include "app/wifi_config_manager.h"
#include "app/backlight_manager.h"
#include "app/app_common.h"
#include "app/app_log.h"
#include "middleware/tuya_mcu_sdk/wifi.h"
#include "middleware/tuya_mcu_sdk/mcu_api.h"
typedef enum
{
    PENDING_MODE_NONE = 0,
    PENDING_MODE_SMART_CONFIG,
    PENDING_MODE_AP
} pending_wifi_mode_t;

static volatile pending_wifi_mode_t s_pending_mode = PENDING_MODE_NONE;
static uint8_t s_last_wifi_state = 0xFFu;

void wifi_config_manager_init(void)
{
    s_pending_mode = PENDING_MODE_NONE;
    s_last_wifi_state = 0xFFu;
}

void wifi_config_manager_enter_smart_config(void)
{
    LOG_INFO("Wi-Fi: Requesting SmartConfig / Pairing mode...");

    /* Set pending mode request for tuya_uart_thread to execute UART write safely */
    s_pending_mode = PENDING_MODE_SMART_CONFIG;

    /* Immediately reflect pairing state and start fast blink */
    xEventGroupSetBits(g_system_state_event_group, SYS_BIT_WIFI_SMART_CONFIG);
    xEventGroupClearBits(g_system_state_event_group, SYS_BIT_WIFI_CONNECTED | SYS_BIT_WIFI_AP_MODE);

    s_last_wifi_state = SMART_CONFIG_STATE;
    backlight_manager_notify_wifi_status(WIFI_LED_FAST_BLINK);
}

void wifi_config_manager_enter_ap_mode(void)
{
    LOG_INFO("Wi-Fi: Requesting AP configuration mode...");

    /* Set pending mode request for tuya_uart_thread to execute UART write safely */
    s_pending_mode = PENDING_MODE_AP;

    /* Immediately reflect pairing state and start slow blink */
    xEventGroupSetBits(g_system_state_event_group, SYS_BIT_WIFI_AP_MODE);
    xEventGroupClearBits(g_system_state_event_group, SYS_BIT_WIFI_CONNECTED | SYS_BIT_WIFI_SMART_CONFIG);

    s_last_wifi_state = AP_STATE;
    backlight_manager_notify_wifi_status(WIFI_LED_SLOW_BLINK);
}

void wifi_config_manager_process(void)
{
    /* 1. Process pending pairing mode change in the safe context of tuya_uart_thread */
    if (PENDING_MODE_SMART_CONFIG == s_pending_mode)
    {
        s_pending_mode = PENDING_MODE_NONE;
        LOG_INFO("Wi-Fi: Sending SmartConfig mode command to Tuya module...");
        mcu_set_wifi_mode(SMART_CONFIG);
    }
    else if (PENDING_MODE_AP == s_pending_mode)
    {
        s_pending_mode = PENDING_MODE_NONE;
        LOG_INFO("Wi-Fi: Sending AP mode command to Tuya module...");
        mcu_set_wifi_mode(AP_CONFIG);
    }

    /* 2. Check and apply any current Wi-Fi working state updates */
    wifi_config_manager_update_status(mcu_get_wifi_work_state());
}

void wifi_config_manager_update_status(uint8_t work_state)
{
    if (work_state == s_last_wifi_state)
    {
        return;
    }

    bool is_currently_pairing = wifi_config_manager_is_pairing();
    wifi_led_pattern_t pattern = WIFI_LED_OFF;
    bool pattern_valid = false;

    switch (work_state)
    {
        case SMART_CONFIG_STATE:
            LOG_INFO("Wi-Fi state: SmartConfig (EZ Mode)");
            pattern = WIFI_LED_FAST_BLINK;
            pattern_valid = true;
            xEventGroupSetBits(g_system_state_event_group, SYS_BIT_WIFI_SMART_CONFIG);
            xEventGroupClearBits(g_system_state_event_group, SYS_BIT_WIFI_CONNECTED | SYS_BIT_WIFI_AP_MODE);
            s_last_wifi_state = work_state;
            break;

        case AP_STATE:
            LOG_INFO("Wi-Fi state: AP Mode");
            pattern = WIFI_LED_SLOW_BLINK;
            pattern_valid = true;
            xEventGroupSetBits(g_system_state_event_group, SYS_BIT_WIFI_AP_MODE);
            xEventGroupClearBits(g_system_state_event_group, SYS_BIT_WIFI_CONNECTED | SYS_BIT_WIFI_SMART_CONFIG);
            s_last_wifi_state = work_state;
            break;

        case SMART_AND_AP_STATE:
            LOG_INFO("Wi-Fi state: Smart & AP Dual Mode");
            pattern = WIFI_LED_FAST_BLINK;
            pattern_valid = true;
            xEventGroupSetBits(g_system_state_event_group, SYS_BIT_WIFI_SMART_CONFIG);
            xEventGroupClearBits(g_system_state_event_group, SYS_BIT_WIFI_CONNECTED | SYS_BIT_WIFI_AP_MODE);
            s_last_wifi_state = work_state;
            break;

        case WIFI_NOT_CONNECTED:
            LOG_INFO("Wi-Fi state: Disconnected / Scanning");
            if (is_currently_pairing)
            {
                /* When actively in pairing mode (SmartConfig or AP mode), the module is
                 * not connected to router by definition. Do NOT cancel pairing mode
                 * or overwrite the fast/slow blink! */
                LOG_INFO("Wi-Fi: Actively pairing; retaining pairing LED blink.");
            }
            else
            {
                pattern = WIFI_LED_PERIODIC_SINGLE_BLINK;
                pattern_valid = true;
                xEventGroupClearBits(g_system_state_event_group,
                                     SYS_BIT_WIFI_CONNECTED | SYS_BIT_WIFI_SMART_CONFIG | SYS_BIT_WIFI_AP_MODE);
            }
            s_last_wifi_state = work_state;
            break;

        case WIFI_CONNECTED:
            LOG_INFO("Wi-Fi state: Connected to Router");
            /* Successfully connected to router; pairing succeeded */
            pattern = WIFI_LED_PERIODIC_SINGLE_BLINK;
            pattern_valid = true;
            xEventGroupSetBits(g_system_state_event_group, SYS_BIT_WIFI_CONNECTED);
            xEventGroupClearBits(g_system_state_event_group, SYS_BIT_WIFI_SMART_CONFIG | SYS_BIT_WIFI_AP_MODE);
            s_last_wifi_state = work_state;
            break;

        case WIFI_CONN_CLOUD:
            LOG_INFO("Wi-Fi state: Connected to Tuya Cloud");
            /* Per spec: Wi-Fi status LED is turned OFF when connected to cloud */
            pattern = WIFI_LED_OFF;
            pattern_valid = true;
            xEventGroupSetBits(g_system_state_event_group, SYS_BIT_WIFI_CONNECTED);
            xEventGroupClearBits(g_system_state_event_group, SYS_BIT_WIFI_SMART_CONFIG | SYS_BIT_WIFI_AP_MODE);
            s_last_wifi_state = work_state;
            break;

        case WIFI_LOW_POWER:
            LOG_INFO("Wi-Fi state: Low Power Standby (Connected to Tuya Cloud)");
            /* Per spec: Wi-Fi status LED is turned OFF when connected to cloud */
            pattern = WIFI_LED_OFF;
            pattern_valid = true;
            xEventGroupSetBits(g_system_state_event_group, SYS_BIT_WIFI_CONNECTED);
            xEventGroupClearBits(g_system_state_event_group, SYS_BIT_WIFI_SMART_CONFIG | SYS_BIT_WIFI_AP_MODE);
            s_last_wifi_state = work_state;
            break;

        default:
            LOG_WARNING("Wi-Fi state: Unknown (0x%02X)", work_state);
            if (!is_currently_pairing)
            {
                pattern = WIFI_LED_PERIODIC_SINGLE_BLINK;
                pattern_valid = true;
            }
            s_last_wifi_state = work_state;
            break;
    }

    if (pattern_valid)
    {
        backlight_manager_notify_wifi_status(pattern);
    }
}

bool wifi_config_manager_is_connected(void)
{
    EventBits_t bits = xEventGroupGetBits(g_system_state_event_group);
    return ((bits & SYS_BIT_WIFI_CONNECTED) != 0u);
}

bool wifi_config_manager_is_pairing(void)
{
    EventBits_t bits = xEventGroupGetBits(g_system_state_event_group);
    return ((bits & (SYS_BIT_WIFI_SMART_CONFIG | SYS_BIT_WIFI_AP_MODE)) != 0u);
}

