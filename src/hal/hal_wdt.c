/*
 * hal_wdt.c
 * See hal_wdt.h.
 */

#include "hal/hal_wdt.h"

void hal_wdt_init(void)
{
    R_WDT_Open(&g_wdt0_ctrl, &g_wdt0_cfg);
}

void hal_wdt_refresh(void)
{
    R_WDT_Refresh(&g_wdt0_ctrl);
}