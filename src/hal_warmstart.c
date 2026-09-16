/*
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "hal_data.h"
#include "app/app_common.h"     /* ADDED: for app_queue_init(), event group handle/bits */

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);

FSP_CPP_FOOTER

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart (bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;

        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
    }

#if BSP_CFG_OSPI_B_STARTUP_ENABLED && defined(BSP_CFG_OSPI_B_STARTUP_FN)
    if (BSP_WARM_START_POST_CLOCK == event)
    {
        /* Setup OSPI_B SiP flash and initialize it. */
        R_BSP_OspiBInit(BSP_CFG_OSPI_B_STARTUP_FN, true);
    }
#endif

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins. */
        R_IOPORT_Open(&IOPORT_CFG_CTRL, &IOPORT_CFG_NAME);

#if BSP_CFG_SDRAM_ENABLED

        /* Setup SDRAM and initialize it. Must configure pins first. */
        R_BSP_SdramInit(true);
#endif

        /* ADDED -- this is the last point guaranteed to run before
         * ra_gen/main.c creates and starts every configured Thread, so
         * it's the correct place for one-time IPC init that thread
         * entry functions depend on from their very first iteration.
         *
         * Event groups/mutexes/semaphores created via the Configurator
         * wizard (New Object, Static allocation) are already live by
         * this point -- FSP's g_common_init() runs even earlier, as
         * part of C runtime init, before BSP_WARM_START_POST_C fires.
         * Only the 4 hand-rolled queues (see app_queue_init.c and the
         * ownership note in app_common.h) need an explicit call here.
         */
        bool queues_ok = app_queue_init();
 
        if (!queues_ok)
        {
            /* Indicates a coding mistake in app_queue_init.c (e.g. a
             * NULL static buffer pointer), not a runtime resource
             * shortage -- static buffers are compile-time arrays, so
             * failure here is a bug to catch under a debugger, not an
             * out-of-memory condition to recover from. Halt with the
             * watchdog un-refreshed so the MCU resets and retries.
             */
            __disable_irq();
            for (;;)
            {
                /* Watchdog will reset us once its timeout elapses --
                 * it never gets a chance to be refreshed because the
                 * scheduler never starts. */
            }
        }
 
        /* Default system state before the Wi-Fi module reports
         * otherwise -- backlight defaults to enabled per spec's
         * "Back-Light ON State" default behavior; Wi-Fi bits stay
         * clear until the Tuya UART thread hears from the module.
         */
        xEventGroupSetBits(g_system_state_event_group, SYS_BIT_BACKLIGHT_ENABLED);
    }
}
