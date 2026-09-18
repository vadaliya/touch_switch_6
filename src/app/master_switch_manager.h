/*
 * master_switch_manager.h
 *
 * Switch-1 Master Switch Manager for Touch Switch 6 project.
 *
 * When Switch-1 is configured as Master Switch (via 15s hold):
 * - Switch-1 short press does NOT toggle Relay 1.
 * - Instead, Switch-1 acts as a Master ON/OFF for SW2..SW6.
 * - If any controlled switch is ON: captures the ON snapshot and turns all OFF.
 * - If all controlled switches are OFF: restores the previous snapshot (if available).
 * - If user turns off all switches manually, the snapshot is cleared.
 * - Configuration is persisted in Data Flash Block 0 at offset 0x120.
 */

#ifndef MASTER_SWITCH_MANAGER_H
#define MASTER_SWITCH_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#define MASTER_SWITCH_EEPROM_OFFSET  (0x120u)
#define MASTER_SWITCH_MAGIC          (0xD4u)

/**
 * master_switch_manager_init
 * Loads Master Switch registration status from Data Flash (offset 0x120).
 */
void master_switch_manager_init(void);

/**
 * master_switch_manager_is_enabled
 * @return true if Switch-1 is currently configured as Master Switch.
 */
bool master_switch_manager_is_enabled(void);

/**
 * master_switch_manager_toggle_mode
 * Toggles Switch-1 between regular switch and Master Switch (15s hold).
 * Persists status to Data Flash Block 0.
 */
void master_switch_manager_toggle_mode(void);

/**
 * master_switch_manager_press
 * Handles Switch-1 activation when configured as Master Switch.
 */
void master_switch_manager_press(void);

/**
 * master_switch_manager_notify_manual_change
 * Informs Master Switch Manager of manual switch state transitions.
 * Clears snapshot if all outputs are turned off manually.
 */
void master_switch_manager_notify_manual_change(void);

/**
 * master_switch_manager_reset
 * Resets Master Switch status to regular switch (called on Factory Reset).
 */
void master_switch_manager_reset(void);

#endif /* MASTER_SWITCH_MANAGER_H */

