/*
 * state_persistence_manager.h
 *
 * State persistence manager for 6 Light Switches on Renesas RA2E1.
 * Uses Data Flash Block 1 (offset 0x400) for append-log wear leveling.
 */

#ifndef STATE_PERSISTENCE_MANAGER_H
#define STATE_PERSISTENCE_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "drivers/relay_driver.h"

#define STATE_PERSIST_BLOCK_OFFSET   (0x400u)   /* Data Flash Block 1 (1024 bytes) */
#define STATE_PERSIST_BLOCK_SIZE     (1024u)
#define STATE_PERSIST_RECORD_SIZE    (4u)
#define STATE_PERSIST_MAX_RECORDS    (STATE_PERSIST_BLOCK_SIZE / STATE_PERSIST_RECORD_SIZE) /* 256 */

#define STATE_PERSIST_MAGIC          (0xA5u)

#define POWER_RECOVERY_EEPROM_OFFSET (0x110u)   /* Data Flash Block 0 (offset 0x110) */
#define POWER_RECOVERY_MAGIC         (0xC3u)

typedef enum
{
    POWER_RECOVERY_OFF    = 0,
    POWER_RECOVERY_ON     = 1,
    POWER_RECOVERY_MEMORY = 2,
} power_recovery_mode_t;

/**
 * state_persistence_manager_init
 * Scans Data Flash Block 1 for the most recent valid power-on state record.
 * If found, populates *out_light_mask and returns true.
 */
bool state_persistence_manager_init(uint8_t *out_light_mask);

/**
 * state_persistence_manager_save
 * Writes the current 6-switch light mask to the next available slot in Block 1.
 * @param light_mask  bits 0..5 for SW1..SW6
 * @return true on success
 */
bool state_persistence_manager_save(uint8_t light_mask);

/**
 * state_persistence_manager_restore_all
 * Restores light switches (SW1..SW6) per the configured power recovery mode (OFF, ON, or MEMORY).
 */
void state_persistence_manager_restore_all(void);

/**
 * Power Recovery Mode Management (Stored in Data Flash Block 0 at 0x110)
 */
power_recovery_mode_t state_persistence_manager_get_power_recovery_mode(void);
bool                  state_persistence_manager_set_power_recovery_mode(power_recovery_mode_t mode);

#endif /* STATE_PERSISTENCE_MANAGER_H */
