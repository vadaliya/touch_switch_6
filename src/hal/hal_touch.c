/*
 * hal_touch.c
 *
 * See hal_touch.h. No custom callback registration -- QE's generated
 * code already sets g_qe_touch_flag internally when a scan completes
 * (see qe_touch_config.c, fully QE-managed/regenerated -- don't
 * hand-edit it). This file just polls that flag in an RTOS-friendly
 * way instead of QE's sample busy-wait.
 */

#include "hal/hal_touch.h"
#include "app/app_log.h"
#include "app/app_common.h"
#include "hal/hal_uart.h"
#include "common_data.h"
#include "FreeRTOS.h"
#include "task.h"

#define TOUCH_POLL_INTERVAL_MS   (2u)   /* yields between checks -- short enough that
                                          * hal_touch_scan_and_wait()'s timeout is still
                                          * respected with reasonable precision */

static bool s_touch_ready = false;

void hal_touch_init(void)
{
    fsp_err_t err = RM_TOUCH_Open(G_TOUCH_INSTANCE.p_ctrl, G_TOUCH_INSTANCE.p_cfg);
    if (FSP_SUCCESS == err)
    {
        LOG_INFO("Touch middleware opened");
    }
    else
    {
        LOG_ERROR("Touch open failed err=%d", (int) err);
    }
    LOG_INFO("Touch tuning must complete before DataGet; 6002 means incomplete tuning");

#if (QE_TUNNING_TESTING != 0)
    /* QE Touch Tuning Enabled: route g_uart_qe callback to QE Touch tuning */
    (void) R_SCI_UART_CallbackSet(&g_uart_qe_ctrl, touch_uart_callback, NULL, NULL);
    LOG_INFO("UART routed to QE Touch Tuning mode");
#else
    /* QE Touch Tuning Disabled: route g_uart_qe callback to Tuya Wi-Fi module */
    uint32_t remaining_bytes = 0u;
    (void) R_SCI_UART_ReadStop(&g_uart_qe_ctrl, &remaining_bytes);
    (void) R_SCI_UART_CallbackSet(&g_uart_qe_ctrl, hal_uart_callback, NULL, NULL);
#endif
}

bool hal_touch_scan_and_wait(uint32_t timeout_ms)
{
    fsp_err_t err = RM_TOUCH_ScanStart(G_TOUCH_INSTANCE.p_ctrl);

    if (FSP_SUCCESS != err)
    {
        LOG_ERROR("Touch scan start failed err=%d", (int) err);
        return false;
    }

    uint32_t waited_ms = 0u;

    while (0 == g_qe_touch_flag)
    {
        if (waited_ms >= timeout_ms)
        {
            LOG_WARNING("Touch scan timeout after %u ms", timeout_ms);
            return false; /* timed out -- scan presumably still in progress */
        }

        vTaskDelay(pdMS_TO_TICKS(TOUCH_POLL_INTERVAL_MS));
        waited_ms += TOUCH_POLL_INTERVAL_MS;
    }

    g_qe_touch_flag = 0; /* consume the flag, matching QE's own sample pattern */
    return true;
}

fsp_err_t hal_touch_get_button_status(uint64_t * p_button_status, uint16_t * p_slider_position)
{
    uint64_t button_status   = 0u;
    uint16_t slider_position = TOUCH_OFF_VALUE;
    static bool s_tuning_logged = false;

    fsp_err_t err = RM_TOUCH_DataGet(G_TOUCH_INSTANCE.p_ctrl, &button_status, &slider_position, NULL);

    if (FSP_ERR_CTSU_INCOMPLETE_TUNING == err)
    {
        if (!s_tuning_logged)
        {
            s_tuning_logged = true;
            LOG_INFO("Touch initial tuning in progress...");
        }
        if (p_button_status != NULL)
        {
            *p_button_status = 0u;
        }
        if (p_slider_position != NULL)
        {
            *p_slider_position = TOUCH_OFF_VALUE;
        }
        return err;
    }

    if (FSP_SUCCESS != err)
    {
        LOG_ERROR("Touch data read failed err=%d", (int) err);
        if (p_button_status != NULL)
        {
            *p_button_status = 0u;
        }
        if (p_slider_position != NULL)
        {
            *p_slider_position = TOUCH_OFF_VALUE;
        }
        return err;
    }

    if (!s_touch_ready)
    {
        s_touch_ready = true;
        LOG_INFO("Touch initial tuning complete");
    }

    if (p_button_status != NULL)
    {
        *p_button_status = button_status;
    }

    if (p_slider_position != NULL)
    {
        *p_slider_position = slider_position;
    }

    return FSP_SUCCESS;
}
