/*
 * tuya_uart_thread_entry.c
 *
 * Owns the UART link to the Tuya Wi-Fi module (Tuya MCU protocol).
 * Drains RX ring buffer, executes Tuya MCU SDK frame processor,
 * processes outbound DP change queue, monitors Wi-Fi connection state,
 * ticks countdown timers, and sets watchdog heartbeat.
 */

#include "app/app_common.h"
#include "hal/hal_uart.h"
#include "middleware/tuya_mcu_sdk/wifi.h"
#include "middleware/tuya_mcu_sdk/mcu_api.h"
#include "app/tuya_dp_handlers.h"
#include "app/countdown_manager.h"
#include "app/wifi_config_manager.h"
#include "app/factory_reset_manager.h"
#include "app/app_log.h"

extern void tuya_uart_thread_entry(void *pvParameters);

void tuya_uart_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    LOG_INFO("Tuya UART Thread: Initializing UART and Tuya MCU SDK...");

    /* Initialize UART hardware instance */
    hal_uart_init();

    /* Initialize Tuya MCU SDK */
    wifi_protocol_init();

    /* Initialize Managers */
    countdown_manager_init();
    wifi_config_manager_init();
    factory_reset_manager_init();

    TickType_t last_periodic_tick = xTaskGetTickCount();
    uint8_t rx_buf[32];

    LOG_INFO("Tuya UART Thread: Ready and entering processing loop.");

    for (;;)
    {
        /* Block on UART RX semaphore with a 20 ms timeout */
        (void) xSemaphoreTake(g_uart_rx_semaphore, pdMS_TO_TICKS(20));

        /* Drain all available bytes from UART RX ring buffer into Tuya SDK */
        while (hal_uart_rx_available() > 0u)
        {
            uint32_t count = hal_uart_rx_read(rx_buf, sizeof(rx_buf));
            for (uint32_t i = 0u; i < count; i++)
            {
                uart_receive_input(rx_buf[i]);
            }
        }

        /* Run Tuya protocol state machine to parse incoming packets */
        wifi_uart_service();

        /* Process outbound DP updates queued by other threads */
        tuya_dp_process_queue();

        /* Process pending Wi-Fi pairing mode requests and incoming state changes immediately */
        wifi_config_manager_process();

        /* Periodic 1000 ms tasks */
        TickType_t current_tick = xTaskGetTickCount();
        if ((current_tick - last_periodic_tick) >= pdMS_TO_TICKS(1000))
        {
            uint32_t elapsed_ms = (uint32_t)((current_tick - last_periodic_tick) * portTICK_PERIOD_MS);
            last_periodic_tick = current_tick;

            /* Advance auto-off countdown timers */
            countdown_manager_tick(elapsed_ms);
        }

        /* Watchdog heartbeat check-in */
        xEventGroupSetBits(g_heartbeat_event_group, HEARTBEAT_BIT_TUYA_UART);
    }
}
