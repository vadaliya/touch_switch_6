/*
 * hal_touch.c
 *
 * *** TEMPORARY STUB -- swap back to the real implementation once ***
 * *** hardware is available and QE tuning/Output Parameter Files  ***
 * *** has been completed (see project chat history for why this   ***
 * *** is gated behind a live debug session on real hardware).     ***
 *
 * This stub has NO dependency on g_touch0 (or any QE-generated
 * symbol), so the project builds cleanly without the Touch (rm_touch)
 * config being finished. touch_driver_scan() will simply see "nothing
 * touched" every cycle -- the rest of the firmware (relay driver,
 * everything built after this point) is unaffected and can be
 * developed/tested independently of the touch subsystem.
 *
 * TO SWITCH BACK: once QE has generated g_touch0_ctrl/g_touch0_cfg,
 * replace this file's body with the real R_TOUCH_Open/ScanStart/
 * DataGet version (kept in the project chat history) -- hal_touch.h's
 * function signatures don't change, so touch_driver.c needs no edits
 * either way.
 */

#include "hal/hal_touch.h"
#include "app/app_common.h"

void hal_touch_init(void)
{
    /* No-op -- nothing to open until g_touch0 exists. */
}

bool hal_touch_scan_and_wait(uint32_t timeout_ms)
{
    /* Stand in for the real scan's timing behavior (so touch_thread's
     * loop period stays realistic) without touching any hardware or
     * QE-generated symbol. Always reports "scan complete, no data" --
     * touch_driver_scan() will see an all-zero button status, i.e.
     * "nothing touched", every cycle.
     */
    vTaskDelay(pdMS_TO_TICKS(timeout_ms > 20u ? 20u : timeout_ms));
    return true;
}

uint64_t hal_touch_get_button_status(void)
{
    return 0u; /* nothing touched -- stub */
}