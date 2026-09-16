/*
 * system_thread_entry.c
 *
 * FSP-generated once for the "System Thread" (symbol: system_thread).
 *
 * Lowest-priority thread. Confirms every critical thread has set its
 * heartbeat bit since the last check, and only then refreshes the
 * hardware watchdog (20 s timeout, per spec). If any bit is missing,
 * the refresh is withheld and the WDT resets the MCU.
 *
 * Also demonstrates reading the system-state event group, which other
 * threads set/clear to broadcast Wi-Fi status and backlight enable --
 * useful here if you want the System thread to, say, only certain
 * checks while Wi-Fi is mid-pairing.
 */

#include "app/app_common.h"

#define SYSTEM_CHECK_PERIOD_MS   (2000u)

void system_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    /* TODO: hal_wdt_init(); -- enable internal WDT (g_wdt0), 20 s timeout,
     * if not already brought up by BSP init before the scheduler starts.
     */

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_CHECK_PERIOD_MS));

        /* EVENT GROUP: read heartbeat bits set by the other 4 threads
         * since the last check.
         */
        EventBits_t heartbeat_bits = xEventGroupGetBits(g_heartbeat_event_group);

        if ((heartbeat_bits & HEARTBEAT_ALL_BITS) == HEARTBEAT_ALL_BITS)
        {
            /* Every critical thread reported healthy -- clear for the
             * next window, then refresh the watchdog.
             */
            xEventGroupClearBits(g_heartbeat_event_group, HEARTBEAT_ALL_BITS);

            /* TODO: hal_wdt_refresh(); */
        }
        else
        {
            /* At least one thread failed to check in -- do NOT refresh.
             * Leave bits as-is (don't clear) so it's inspectable if a
             * debugger catches the reset. The WDT will reset the MCU
             * once its 20 s window elapses; on restart, EEPROM-backed
             * state is restored and Wi-Fi reconnects per spec.
             */
        }

        /* EVENT GROUP (read-only example): check Wi-Fi status bits set
         * by tuya_uart_thread when the module reports its state. Useful
         * for any System-thread logic that should behave differently
         * during Wi-Fi provisioning (e.g. suppressing a low-priority
         * diagnostic while pairing is in progress):
         *
         *   EventBits_t sys_bits = xEventGroupGetBits(g_system_state_event_group);
         *   if (sys_bits & SYS_BIT_WIFI_SMART_CONFIG) { ... }
         */
    }
}
