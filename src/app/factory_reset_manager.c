/*
 * factory_reset_manager.c
 *
 * Implements factory reset sequence.
 */

#include "app/factory_reset_manager.h"
#include "app/light_manager.h"
#include "app/backlight_manager.h"
#include "app/countdown_manager.h"
#include "app/ir_learning_manager.h"
#include "app/master_switch_manager.h"
#include "app/state_persistence_manager.h"
#include "drivers/eeprom_driver.h"
#include "app/app_common.h"
#include "app/app_log.h"
#include "middleware/tuya_mcu_sdk/mcu_api.h"

void factory_reset_manager_init(void)
{
    /* No persistent state needed */
}

void factory_reset_manager_execute(void)
{
    LOG_INFO("Factory Reset: Starting full system restore to defaults...");

    /* 1. Turn all outputs OFF */
    light_manager_all_off();

    /* 2. Cancel all countdown timers */
    countdown_manager_cancel_all();

    /* 3. Clear child lock */
    xEventGroupClearBits(g_system_state_event_group, SYS_BIT_CHILD_LOCK);

    /* 4. Erase Data Flash Block 1 (State Persistence log at 0x400) */
    if (xSemaphoreTake(g_eeprom_mutex, pdMS_TO_TICKS(500)) == pdTRUE)
    {
        (void) eeprom_erase_blocking(STATE_PERSIST_BLOCK_OFFSET, 1u, 2000u);

        /* 5. Erase Data Flash Block 0 (IR codes at 0x000 & Backlight config at 0x100) */
        (void) eeprom_erase_blocking(0u, 1u, 2000u);

        xSemaphoreGive(g_eeprom_mutex);
    }

    /* 6. Re-initialize managers with defaults */
    backlight_manager_init();
    ir_learning_manager_init();
    master_switch_manager_reset();

    /* 7. Refresh all LEDs to reflect clean default state */
    backlight_manager_refresh_all();

    /* 8. Reset Tuya Wi-Fi module into SmartConfig pairing */
    LOG_INFO("Factory Reset: Resetting Wi-Fi module...");
    mcu_reset_wifi();

    LOG_INFO("Factory Reset: Complete!");
}

