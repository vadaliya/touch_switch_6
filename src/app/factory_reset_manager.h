/*
 * factory_reset_manager.h
 *
 * Coordinates full factory reset sequence for the Touch Switch 6:
 * - Wipes learned IR codes and scenes in Data Flash Block 0
 * - Restores backlight defaults in Data Flash Block 0
 * - Erases state retention log in Data Flash Block 1
 * - Clears auto-off countdown timers
 * - Clears child lock
 * - Turns all 6 light relays OFF
 * - Triggers Tuya Wi-Fi module reset into pairing mode
 */

#ifndef FACTORY_RESET_MANAGER_H
#define FACTORY_RESET_MANAGER_H

#include <stdbool.h>

/**
 * factory_reset_manager_init
 * Initializes the factory reset manager.
 */
void factory_reset_manager_init(void);

/**
 * factory_reset_manager_execute
 * Executes the complete factory reset sequence.
 */
void factory_reset_manager_execute(void);

#endif /* FACTORY_RESET_MANAGER_H */

