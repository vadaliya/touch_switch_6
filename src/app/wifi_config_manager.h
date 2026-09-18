/*
 * wifi_config_manager.h
 *
 * Manages Wi-Fi configuration modes (SmartConfig / AP mode) and
 * translates Tuya Wi-Fi working states into Wi-Fi status LED patterns.
 */

#ifndef WIFI_CONFIG_MANAGER_H
#define WIFI_CONFIG_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * wifi_config_manager_init
 * Initializes internal Wi-Fi state tracking.
 */
void wifi_config_manager_init(void);

/**
 * wifi_config_manager_enter_smart_config
 * Triggers Tuya module reset to enter SmartConfig (EZ) pairing mode.
 * Typically called after a 10s touch long press.
 */
void wifi_config_manager_enter_smart_config(void);

/**
 * wifi_config_manager_enter_ap_mode
 * Configures Tuya module into AP network configuration mode.
 */
void wifi_config_manager_enter_ap_mode(void);

/**
 * wifi_config_manager_process
 * Processes any pending mode change requests (SmartConfig / AP mode) safely
 * in the context of tuya_uart_thread and updates status immediately.
 */
void wifi_config_manager_process(void);

/**
 * wifi_config_manager_update_status
 * Checks the current Tuya module Wi-Fi work state and updates LED pattern
 * and system state bits if changed.
 *
 * @param work_state  Value returned from mcu_get_wifi_work_state()
 */
void wifi_config_manager_update_status(uint8_t work_state);

/**
 * wifi_config_manager_is_connected
 * @return true if connected to cloud
 */
bool wifi_config_manager_is_connected(void);

/**
 * wifi_config_manager_is_pairing
 * @return true if currently in SmartConfig or AP pairing mode
 */
bool wifi_config_manager_is_pairing(void);

#endif /* WIFI_CONFIG_MANAGER_H */

