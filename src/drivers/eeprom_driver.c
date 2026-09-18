/*
 * eeprom_driver.c
 *
 * See eeprom_driver.h, especially the async usage contract at the top.
 */

#include "drivers/eeprom_driver.h"
#include "hal/hal_flash.h"
#include "app/app_common.h"

#define EEPROM_DATA_FLASH_BASE   (BSP_FEATURE_FLASH_DATA_FLASH_START)

void eeprom_driver_init(void)
{
    hal_flash_init();
}

bool eeprom_read(uint32_t offset, uint8_t *dst, uint32_t length, uint32_t timeout_ms)
{
    if (xSemaphoreTake(g_eeprom_mutex, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
    {
        return false;
    }

    hal_flash_read(EEPROM_DATA_FLASH_BASE + offset, dst, length);

    xSemaphoreGive(g_eeprom_mutex);
    return true;
}

#define EEPROM_MAX_WRITE_CHUNK   (64u)
static uint8_t s_eeprom_write_buf[EEPROM_MAX_WRITE_CHUNK];

bool eeprom_write_blocking(uint32_t offset, const uint8_t *data, uint32_t length, uint32_t timeout_ms)
{
    if ((data == NULL) || (length == 0u) || (length > EEPROM_MAX_WRITE_CHUNK))
    {
        return false;
    }

    if (xSemaphoreTake(g_eeprom_mutex, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
    {
        LOG_ERROR("EEPROM write: mutex take timeout");
        return false;
    }

    /* Copy to static buffer so RAM source is guaranteed valid and aligned throughout BGO */
    memcpy(s_eeprom_write_buf, data, length);

    /* Drain any stale semaphore token from prior operations */
    (void) xSemaphoreTake(g_flash_op_complete_semaphore, 0);

    bool ok = hal_flash_write_start(EEPROM_DATA_FLASH_BASE + offset, s_eeprom_write_buf, length);

    if (ok)
    {
        ok = hal_flash_wait_complete(timeout_ms);
        if (!ok)
        {
            LOG_ERROR("EEPROM write: BGO wait timeout (%lu ms) at offset 0x%04lX",
                      (unsigned long) timeout_ms, (unsigned long) offset);
        }
    }

    xSemaphoreGive(g_eeprom_mutex);
    return ok;
}

bool eeprom_erase_blocking(uint32_t offset, uint32_t num_blocks, uint32_t timeout_ms)
{
    if (xSemaphoreTake(g_eeprom_mutex, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
    {
        LOG_ERROR("EEPROM erase: mutex take timeout");
        return false;
    }

    /* Drain any stale semaphore token from prior operations */
    (void) xSemaphoreTake(g_flash_op_complete_semaphore, 0);

    bool ok = hal_flash_erase_block_start(EEPROM_DATA_FLASH_BASE + offset, num_blocks);

    if (ok)
    {
        ok = hal_flash_wait_complete(timeout_ms);
        if (!ok)
        {
            LOG_ERROR("EEPROM erase: BGO wait timeout (%lu ms) at offset 0x%04lX",
                      (unsigned long) timeout_ms, (unsigned long) offset);
        }
    }

    xSemaphoreGive(g_eeprom_mutex);
    return ok;
}

bool eeprom_write_async_start(uint32_t offset, const uint8_t *data, uint32_t length)
{
    /* 0 timeout: this is the non-blocking entry point, so acquiring
     * exclusivity must not block either -- if busy, fail fast. */
    if (xSemaphoreTake(g_eeprom_mutex, 0) != pdTRUE)
    {
        return false;
    }

    bool started = hal_flash_write_start(EEPROM_DATA_FLASH_BASE + offset, data, length);

    if (!started)
    {
        /* Failed to even start (unexpected -- we just confirmed
         * exclusivity via the mutex) -- release immediately, nothing
         * for the caller to poll/wait for. */
        xSemaphoreGive(g_eeprom_mutex);
    }

    /* On success, mutex stays HELD -- caller must poll/wait. */
    return started;
}

bool eeprom_erase_async_start(uint32_t offset, uint32_t num_blocks)
{
    if (xSemaphoreTake(g_eeprom_mutex, 0) != pdTRUE)
    {
        return false;
    }

    bool started = hal_flash_erase_block_start(EEPROM_DATA_FLASH_BASE + offset, num_blocks);

    if (!started)
    {
        xSemaphoreGive(g_eeprom_mutex);
    }

    return started;
}

bool eeprom_async_poll(void)
{
    if (hal_flash_poll_complete())
    {
        xSemaphoreGive(g_eeprom_mutex);
        return true;
    }

    return false; /* still in progress -- mutex remains held */
}

bool eeprom_async_wait(uint32_t timeout_ms)
{
    if (hal_flash_wait_complete(timeout_ms))
    {
        xSemaphoreGive(g_eeprom_mutex);
        return true;
    }

    return false; /* timed out -- mutex remains held, operation presumed still in flight */
}