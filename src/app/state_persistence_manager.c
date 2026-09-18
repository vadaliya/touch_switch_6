/*
 * state_persistence_manager.c
 *
 * Implements circular append-log state retention in Data Flash Block 1 for 6 switches.
 */

#include "app/state_persistence_manager.h"
#include "app/light_manager.h"
#include "drivers/relay_driver.h"
#include "drivers/eeprom_driver.h"
#include "app/app_log.h"

#include <string.h>

static uint32_t s_next_write_slot   = 0u;
static uint8_t  s_cached_light_mask = 0u;
static bool     s_has_cached        = false;

static bool validate_record(const uint8_t bytes[4], uint8_t *out_light_mask)
{
    if (bytes[0] != STATE_PERSIST_MAGIC)
    {
        return false;
    }

    uint8_t light_mask = bytes[1];
    uint8_t reserved   = bytes[2];
    uint8_t checksum   = bytes[3];

    if ((light_mask & 0xC0u) != 0u)
    {
        return false;
    }

    uint8_t expected_checksum = (uint8_t) (STATE_PERSIST_MAGIC ^ light_mask ^ reserved);
    if (checksum != expected_checksum)
    {
        return false;
    }

    *out_light_mask = light_mask;
    return true;
}

bool state_persistence_manager_init(uint8_t *out_light_mask)
{
    eeprom_driver_init();

    int32_t  last_valid_slot   = -1;
    uint8_t  latest_light_mask = 0u;
    uint32_t first_blank_slot  = STATE_PERSIST_MAX_RECORDS;

    for (uint32_t slot = 0u; slot < STATE_PERSIST_MAX_RECORDS; slot++)
    {
        uint8_t bytes[STATE_PERSIST_RECORD_SIZE];
        uint32_t offset = STATE_PERSIST_BLOCK_OFFSET + (slot * STATE_PERSIST_RECORD_SIZE);

        if (!eeprom_read(offset, bytes, STATE_PERSIST_RECORD_SIZE, 50u))
        {
            continue;
        }

        uint8_t lm = 0u;
        if (validate_record(bytes, &lm))
        {
            last_valid_slot   = (int32_t) slot;
            latest_light_mask = lm;
        }
        else if ((bytes[0] == 0xFFu) && (bytes[1] == 0xFFu) && (bytes[2] == 0xFFu) && (bytes[3] == 0xFFu))
        {
            if (first_blank_slot == STATE_PERSIST_MAX_RECORDS)
            {
                first_blank_slot = slot;
            }
        }
    }

    if (last_valid_slot >= 0)
    {
        s_next_write_slot = (uint32_t) (last_valid_slot + 1);
        if (s_next_write_slot < STATE_PERSIST_MAX_RECORDS)
        {
            uint8_t next_bytes[STATE_PERSIST_RECORD_SIZE];
            uint32_t next_offset = STATE_PERSIST_BLOCK_OFFSET + (s_next_write_slot * STATE_PERSIST_RECORD_SIZE);
            if (eeprom_read(next_offset, next_bytes, STATE_PERSIST_RECORD_SIZE, 50u))
            {
                if ((next_bytes[0] != 0xFFu) || (next_bytes[1] != 0xFFu) ||
                    (next_bytes[2] != 0xFFu) || (next_bytes[3] != 0xFFu))
                {
                    s_next_write_slot = STATE_PERSIST_MAX_RECORDS;
                }
            }
        }

        *out_light_mask     = latest_light_mask;
        s_cached_light_mask = latest_light_mask;
        s_has_cached        = true;

        LOG_INFO("State Restored from Flash: SW Mask=0x%02X (Slot %ld/256)",
                 latest_light_mask, (long) last_valid_slot);
        return true;
    }

    /* No valid record found -- check if slot 0 is blank; if dirty, format Block 1 */
    uint8_t slot0_bytes[STATE_PERSIST_RECORD_SIZE];
    if (eeprom_read(STATE_PERSIST_BLOCK_OFFSET, slot0_bytes, STATE_PERSIST_RECORD_SIZE, 50u))
    {
        if ((slot0_bytes[0] != 0xFFu) || (slot0_bytes[1] != 0xFFu) ||
            (slot0_bytes[2] != 0xFFu) || (slot0_bytes[3] != 0xFFu))
        {
            LOG_INFO("State Persistence: Formatting Block 1 (erasing dirty flash)...");
            eeprom_erase_blocking(STATE_PERSIST_BLOCK_OFFSET, 1u, 2000u);
        }
    }
    s_next_write_slot = 0u;

    *out_light_mask     = 0u;
    s_cached_light_mask = 0u;
    s_has_cached        = true;

    LOG_INFO("No saved state in Flash (Blank). Defaulting to all OFF.");
    return false;
}

bool state_persistence_manager_save(uint8_t light_mask)
{
    light_mask &= 0x3Fu;

    if (s_has_cached && (light_mask == s_cached_light_mask))
    {
        return true; /* Unchanged */
    }

    uint8_t bytes[STATE_PERSIST_RECORD_SIZE];
    bytes[0] = STATE_PERSIST_MAGIC;
    bytes[1] = light_mask;
    bytes[2] = 0u;
    bytes[3] = (uint8_t) (STATE_PERSIST_MAGIC ^ light_mask ^ 0u);

    if (s_next_write_slot >= STATE_PERSIST_MAX_RECORDS)
    {
        LOG_INFO("State Persistence: Block 1 full, erasing block for rollover...");
        if (!eeprom_erase_blocking(STATE_PERSIST_BLOCK_OFFSET, 1u, 2000u))
        {
            LOG_ERROR("State Persistence: Flash Erase failed!");
            return false;
        }
        s_next_write_slot = 0u;
    }

    uint32_t offset = STATE_PERSIST_BLOCK_OFFSET + (s_next_write_slot * STATE_PERSIST_RECORD_SIZE);
    bool ok = eeprom_write_blocking(offset, bytes, STATE_PERSIST_RECORD_SIZE, 1000u);

    if (ok)
    {
        s_cached_light_mask = light_mask;
        s_has_cached        = true;
        s_next_write_slot++;

        LOG_INFO("State Saved to Flash: SW Mask=0x%02X (Slot %lu/256)",
                 light_mask, (unsigned long) s_next_write_slot);
    }
    else
    {
        LOG_ERROR("State Persistence: Flash Write failed at offset 0x%04lX!", (unsigned long) offset);
    }

    return ok;
}

typedef struct
{
    uint8_t magic;    /* 0xC3 */
    uint8_t mode;     /* power_recovery_mode_t */
    uint8_t reserved; /* 0x00 */
    uint8_t checksum; /* magic ^ mode ^ reserved */
} power_recovery_record_t;

static power_recovery_mode_t s_power_recovery_mode   = POWER_RECOVERY_MEMORY;
static bool                  s_power_recovery_loaded = false;

static uint8_t calc_recovery_checksum(const power_recovery_record_t *rec)
{
    return (uint8_t) (rec->magic ^ rec->mode ^ rec->reserved);
}

power_recovery_mode_t state_persistence_manager_get_power_recovery_mode(void)
{
    if (s_power_recovery_loaded)
    {
        return s_power_recovery_mode;
    }

    power_recovery_record_t rec;
    if (eeprom_read(POWER_RECOVERY_EEPROM_OFFSET, (uint8_t *) &rec, sizeof(rec), 100u))
    {
        if ((rec.magic == POWER_RECOVERY_MAGIC) &&
            (rec.checksum == calc_recovery_checksum(&rec)) &&
            (rec.mode <= (uint8_t) POWER_RECOVERY_MEMORY))
        {
            s_power_recovery_mode   = (power_recovery_mode_t) rec.mode;
            s_power_recovery_loaded = true;
            return s_power_recovery_mode;
        }
    }

    s_power_recovery_mode   = POWER_RECOVERY_MEMORY;
    s_power_recovery_loaded = true;
    return s_power_recovery_mode;
}

bool state_persistence_manager_set_power_recovery_mode(power_recovery_mode_t mode)
{
    if (mode > POWER_RECOVERY_MEMORY)
    {
        return false;
    }

    s_power_recovery_mode   = mode;
    s_power_recovery_loaded = true;

    /* Read existing IR codes & scenes (38 bytes at 0x000) */
    uint8_t ir_buf[38];
    bool has_ir = eeprom_read(0u, ir_buf, sizeof(ir_buf), 100u);

    /* Read existing Backlight config (at 0x100) */
    uint8_t bl_buf[16];
    bool has_bl = eeprom_read(0x100u, bl_buf, sizeof(bl_buf), 100u);

    /* Read existing Master Switch config (at 0x120) */
    uint8_t ms_buf[4];
    bool has_ms = eeprom_read(0x120u, ms_buf, sizeof(ms_buf), 100u);

    power_recovery_record_t rec;
    rec.magic    = POWER_RECOVERY_MAGIC;
    rec.mode     = (uint8_t) mode;
    rec.reserved = 0u;
    rec.checksum = calc_recovery_checksum(&rec);

    if (!eeprom_erase_blocking(0u, 1u, 2000u))
    {
        LOG_ERROR("State Persistence: Failed to erase Block 0 for power recovery mode!");
        return false;
    }

    if (has_ir)
    {
        (void) eeprom_write_blocking(0u, ir_buf, sizeof(ir_buf), 1000u);
    }

    if (has_bl)
    {
        (void) eeprom_write_blocking(0x100u, bl_buf, sizeof(bl_buf), 1000u);
    }

    if (has_ms && (ms_buf[0] == 0xD4u))
    {
        (void) eeprom_write_blocking(0x120u, ms_buf, sizeof(ms_buf), 1000u);
    }

    bool ok = eeprom_write_blocking(POWER_RECOVERY_EEPROM_OFFSET, (const uint8_t *) &rec, sizeof(rec), 1000u);
    if (ok)
    {
        LOG_INFO("Power Recovery Mode saved to Flash: %s (%u)",
                 (mode == POWER_RECOVERY_OFF) ? "ALL OFF" :
                 (mode == POWER_RECOVERY_ON)  ? "ALL ON" : "MEMORY",
                 (unsigned) mode);
    }
    return ok;
}

void state_persistence_manager_restore_all(void)
{
    relay_driver_init();

    uint8_t light_mask = 0u;

    /* 1. Read last saved memory state from Block 1 */
    state_persistence_manager_init(&light_mask);

    /* 2. Check configured power recovery mode in Block 0 */
    power_recovery_mode_t recovery_mode = state_persistence_manager_get_power_recovery_mode();

    LOG_INFO("Power Recovery Mode on boot: %s (%u)",
             (recovery_mode == POWER_RECOVERY_OFF) ? "ALL OFF" :
             (recovery_mode == POWER_RECOVERY_ON)  ? "ALL ON" : "MEMORY",
             (unsigned) recovery_mode);

    if (recovery_mode == POWER_RECOVERY_OFF)
    {
        light_manager_apply_mask(0x00u);
        state_persistence_manager_save(0x00u);
    }
    else if (recovery_mode == POWER_RECOVERY_ON)
    {
        light_manager_apply_mask(0x3Fu);
        state_persistence_manager_save(0x3Fu);
    }
    else /* POWER_RECOVERY_MEMORY (default) */
    {
        light_manager_apply_mask(light_mask);
    }
}
