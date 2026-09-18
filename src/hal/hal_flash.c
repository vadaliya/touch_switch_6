/*
 * hal_flash.c
 *
 * See hal_flash.h. API calls verified against Renesas's own r_flash_lp
 * documentation and example code:
 *   R_FLASH_LP_Write(p_ctrl, (uint32_t) src_address, flash_address, num_bytes)
 *   R_FLASH_LP_Erase(p_ctrl, block_address, num_blocks)
 *
 * TODO: this file assumes your generated hal_data.h exposes
 * FLASH_DF_BLOCK_0 and FLASH_DATA_BLOCK_SIZE macros (these are the
 * documented Renesas example names) -- open hal_data.h after
 * generating and confirm these exact names exist for your device;
 * rename here if FSP generated something different for RA2E1.
 */

#include "hal/hal_flash.h"
#include "app/app_common.h"   /* g_flash_op_complete_semaphore */

#include <string.h>

/* Registered as g_flash0's callback in the Configurator (Properties ->
 * Callback -> hal_flash_callback). Providing ANY callback here is what
 * switches r_flash_lp into non-blocking BGO mode for data flash
 * operations -- this is not a separate checkbox, per Renesas's docs.
 */
void hal_flash_callback(flash_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    BaseType_t higher_priority_task_woken = pdFALSE;
    xSemaphoreGiveFromISR(g_flash_op_complete_semaphore, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

static flash_cfg_t s_flash_cfg;

void hal_flash_init(void)
{
    s_flash_cfg = g_flash0_cfg;
    if (s_flash_cfg.ipl == 0u)
    {
        s_flash_cfg.ipl = 2u;
    }

    fsp_err_t err = R_FLASH_LP_Open(&g_flash0_ctrl, &s_flash_cfg);
    if ((err != FSP_SUCCESS) && (err != FSP_ERR_ALREADY_OPEN))
    {
        LOG_ERROR("R_FLASH_LP_Open failed err=%d", (int) err);
    }
}

bool hal_flash_write_start(uint32_t flash_address, const uint8_t *src, uint32_t num_bytes)
{
    fsp_err_t err = R_FLASH_LP_Write(&g_flash0_ctrl, (uint32_t) src, flash_address, num_bytes);
    if (err != FSP_SUCCESS)
    {
        LOG_ERROR("R_FLASH_LP_Write failed err=%d (addr=0x%08lX)", (int) err, (unsigned long) flash_address);
    }
    return (FSP_SUCCESS == err);
}

bool hal_flash_erase_block_start(uint32_t flash_address, uint32_t num_blocks)
{
    fsp_err_t err = R_FLASH_LP_Erase(&g_flash0_ctrl, flash_address, num_blocks);
    if (err != FSP_SUCCESS)
    {
        LOG_ERROR("R_FLASH_LP_Erase failed err=%d (addr=0x%08lX)", (int) err, (unsigned long) flash_address);
    }
    return (FSP_SUCCESS == err);
}

bool hal_flash_wait_complete(uint32_t timeout_ms)
{
    return (xSemaphoreTake(g_flash_op_complete_semaphore, pdMS_TO_TICKS(timeout_ms)) == pdTRUE);
}

bool hal_flash_poll_complete(void)
{
    return (xSemaphoreTake(g_flash_op_complete_semaphore, 0) == pdTRUE);
}

void hal_flash_read(uint32_t flash_address, uint8_t *dst, uint32_t num_bytes)
{
    memcpy(dst, (const void *) flash_address, num_bytes);
}