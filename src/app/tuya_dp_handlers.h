/*
 * tuya_dp_handlers.h
 *
 * Handler implementations for Tuya Data Points (DPs) on the 6-switch module.
 */

#ifndef TUYA_DP_HANDLERS_H
#define TUYA_DP_HANDLERS_H

#include <stdint.h>
#include <stdbool.h>
#include "drivers/rgb_led_driver.h"

/**
 * DP Downlink Handlers
 * Called by protocol.c when a command is received from the Tuya module.
 * Return true on success, false on error.
 */
bool tuya_dp_handle_switch(uint8_t switch_id, bool on);
bool tuya_dp_handle_countdown(uint8_t switch_id, uint32_t seconds);
bool tuya_dp_handle_switch_all(bool on);
bool tuya_dp_handle_relay_status(uint8_t status);
bool tuya_dp_handle_backlight_switch(bool on);
bool tuya_dp_handle_color_values(uint32_t color_val);
bool tuya_dp_handle_set_color(bool target);
bool tuya_dp_handle_bright_per(uint32_t brightness);
bool tuya_dp_handle_child_lock(bool locked);
bool tuya_dp_handle_factory_reset(bool reset);

/**
 * tuya_dp_sync_all
 * Reports the current state of all DPs to the Tuya module / cloud.
 * Called by all_data_update() on cloud request or after Wi-Fi connects.
 */
void tuya_dp_sync_all(void);

/**
 * Outbound DP change notification helpers.
 * These post tuya_dp_cmd_t to g_tuya_dp_queue, safely decoupling
 * callers (Touch Thread, IR RX Thread, Countdown Manager) from UART TX.
 */
void tuya_dp_notify_switch_changed(uint8_t switch_id, bool on);
void tuya_dp_notify_countdown_changed(uint8_t switch_id, uint32_t seconds);
void tuya_dp_notify_switch_all_changed(bool on);
void tuya_dp_notify_child_lock_changed(bool locked);
void tuya_dp_notify_backlight_changed(bool on);
void tuya_dp_notify_brightness_changed(uint8_t brightness);
void tuya_dp_notify_color_changed(rgb_color_t color);

/**
 * tuya_dp_process_queue
 * Drains g_tuya_dp_queue and calls the respective mcu_dp_*_update functions.
 * Must only be called from tuya_uart_thread.
 */
void tuya_dp_process_queue(void);

#endif /* TUYA_DP_HANDLERS_H */
