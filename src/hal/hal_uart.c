/*
 * hal_uart.c
 * See hal_uart.h.
 */

#include "hal/hal_uart.h"
#include "app/app_common.h"   /* g_uart_rx_semaphore, g_uart_tx_complete_semaphore */

static volatile uint8_t  s_rx_ring[UART_RX_RING_BUFFER_SIZE];
static volatile uint32_t s_rx_head = 0u;   /* next write index (ISR-owned) */
static volatile uint32_t s_rx_tail = 0u;   /* next read index (task-owned) */

/* Callback for Tuya Wi-Fi module on SCI9 (g_uart_qe, P109 TX / P110 RX)
 */
void hal_uart_callback(uart_callback_args_t *p_args)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    switch (p_args->event)
    {
        case UART_EVENT_RX_CHAR:
        {
            uint32_t next_head = (s_rx_head + 1u) % UART_RX_RING_BUFFER_SIZE;

            if (next_head != s_rx_tail) /* drop byte silently if ring buffer is full --
                                          * better than corrupting adjacent memory; a
                                          * full buffer here means tuya_uart_thread
                                          * isn't draining fast enough, worth
                                          * investigating if you ever see it happen */
            {
                s_rx_ring[s_rx_head] = (uint8_t) p_args->data;
                s_rx_head = next_head;
            }

            xSemaphoreGiveFromISR(g_uart_rx_semaphore, &higher_priority_task_woken);
            break;
        }

        case UART_EVENT_TX_COMPLETE:
        {
            xSemaphoreGiveFromISR(g_uart_tx_complete_semaphore, &higher_priority_task_woken);
            break;
        }

        default:
        {
            /* UART_EVENT_ERR_* and others not handled specially here --
             * add handling if you need to detect framing/overrun errors
             * at this layer rather than relying on the Tuya protocol's
             * own checksum to catch corruption. */
            break;
        }
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
}

void hal_uart_init(void)
{
    s_rx_head = 0u;
    s_rx_tail = 0u;

#if (QE_TUNNING_TESTING != 0)
    /* QE Touch Tuning Enabled: route g_uart_qe (SCI9) to QE Touch tuning callback */
    if (0 != g_uart_qe_ctrl.open)
    {
        (void) R_SCI_UART_CallbackSet(&g_uart_qe_ctrl, touch_uart_callback, NULL, NULL);
    }
    else
    {
        static uart_cfg_t s_qe_uart_cfg;
        s_qe_uart_cfg = g_uart_qe_cfg;
        s_qe_uart_cfg.p_callback = touch_uart_callback;
        s_qe_uart_cfg.p_context  = NULL;
        (void) R_SCI_UART_Open(&g_uart_qe_ctrl, &s_qe_uart_cfg);
    }
#else
    /* QE Touch Tuning Disabled: route g_uart_qe (SCI9) to Tuya Wi-Fi module callback */
    if (0 != g_uart_qe_ctrl.open)
    {
        uint32_t remaining_bytes = 0u;
        (void) R_SCI_UART_ReadStop(&g_uart_qe_ctrl, &remaining_bytes);
        (void) R_SCI_UART_CallbackSet(&g_uart_qe_ctrl, hal_uart_callback, NULL, NULL);
    }
    else
    {
        static uart_cfg_t s_tuya_uart_cfg;
        s_tuya_uart_cfg = g_uart_qe_cfg;
        s_tuya_uart_cfg.p_callback = hal_uart_callback;
        s_tuya_uart_cfg.p_context  = NULL;
        (void) R_SCI_UART_Open(&g_uart_qe_ctrl, &s_tuya_uart_cfg);
    }
#endif
}

bool hal_uart_send_blocking(const uint8_t *data, uint32_t length, uint32_t timeout_ms)
{
    if (!hal_uart_send_start(data, length))
    {
        return false;
    }

    return hal_uart_send_wait_complete(timeout_ms);
}

bool hal_uart_send_start(const uint8_t *data, uint32_t length)
{
#if (QE_TUNNING_TESTING != 0)
    FSP_PARAMETER_NOT_USED(data);
    FSP_PARAMETER_NOT_USED(length);
    return false;
#else
    fsp_err_t err = R_SCI_UART_Write(&g_uart_qe_ctrl, data, length);
    return (FSP_SUCCESS == err);
#endif
}

bool hal_uart_send_wait_complete(uint32_t timeout_ms)
{
    return (xSemaphoreTake(g_uart_tx_complete_semaphore, pdMS_TO_TICKS(timeout_ms)) == pdTRUE);
}

bool hal_uart_send_poll_complete(void)
{
    return (xSemaphoreTake(g_uart_tx_complete_semaphore, 0) == pdTRUE);
}

uint32_t hal_uart_rx_available(void)
{
    /* s_rx_head is ISR-written -- snapshot once for a consistent
     * calculation rather than reading it twice. */
    uint32_t head = s_rx_head;
    uint32_t tail = s_rx_tail;

    return (head >= tail) ? (head - tail) : (UART_RX_RING_BUFFER_SIZE - tail + head);
}

uint32_t hal_uart_rx_read(uint8_t *dst, uint32_t max_length)
{
    uint32_t count = 0u;

    while ((count < max_length) && (s_rx_tail != s_rx_head))
    {
        dst[count] = s_rx_ring[s_rx_tail];
        s_rx_tail  = (s_rx_tail + 1u) % UART_RX_RING_BUFFER_SIZE;
        count++;
    }

    return count;
}