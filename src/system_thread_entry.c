/*
 * system_thread_entry.c
 *
 * FSP-generated once for the "System Thread" (symbol: system_thread).
 *
 * Lowest-priority thread. Confirms every critical thread has set its
 * heartbeat bit since the last check, and only then refreshes the
 * hardware watchdog (20 s timeout, per spec). If any bit is missing,
 * the refresh is withheld and the WDT resets the MCU.
 */

#include "app/app_common.h"
#include "hal/hal_wdt.h"

#define SYSTEM_CHECK_PERIOD_MS   (2000u)

extern void system_thread_entry(void *pvParameters);


void system_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    /* g_common_init() has completed before this thread enters user code. */
    xEventGroupSetBits(g_system_state_event_group, SYS_BIT_BACKLIGHT_ENABLED);

    hal_wdt_init(); /* configures the WDT; countdown actually starts on the
                      * first hal_wdt_refresh() call below -- see hal_wdt.h */

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
             * next window, then refresh (and, on the very first pass,
             * start) the watchdog.
             */
            xEventGroupClearBits(g_heartbeat_event_group, HEARTBEAT_ALL_BITS);

            hal_wdt_refresh();
        }
        else
        {
            /* At least one thread failed to check in -- do NOT refresh.
             * Leave bits as-is (don't clear) so it's inspectable if a
             * debugger catches the reset. The WDT will reset the MCU
             * once its window elapses; on restart, EEPROM-backed
             * state is restored and Wi-Fi reconnects per spec.
             */

            /* TODO: Optional diagnostic logging / fault capture before watchdog reset:
             *
             * EventBits_t missing_bits = HEARTBEAT_ALL_BITS & (~heartbeat_bits);
             * LOG_ERROR("WDT: Heartbeat missing (mask=0x%02X):%s%s%s%s",
             *           (unsigned) missing_bits,
             *           (missing_bits & HEARTBEAT_BIT_TOUCH)     ? " [TOUCH]" : "",
             *           (missing_bits & HEARTBEAT_BIT_IR_RX)     ? " [IR_RX]" : "",
             *           (missing_bits & HEARTBEAT_BIT_RGB)       ? " [RGB]" : "",
             *           (missing_bits & HEARTBEAT_BIT_TUYA_UART) ? " [TUYA_UART]" : "");
             *
             * // Optional: Immediate forced software reset instead of waiting for hardware WDT timeout:
             * // __NVIC_SystemReset();
             */
        }

        /* EVENT GROUP (read-only example): check Wi-Fi status bits set
         * by tuya_uart_thread when the module reports its state. Useful
         * for any System-thread logic that should behave differently
         * during Wi-Fi provisioning:
         *
         *   EventBits_t sys_bits = xEventGroupGetBits(g_system_state_event_group);
         *   if (sys_bits & SYS_BIT_WIFI_SMART_CONFIG) { ... }
         */
    }
}