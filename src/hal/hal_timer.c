/*
 * hal_timer.c
 *
 * Free-running microsecond timer implementation using AGT0.
 *
 * RA2E1 Clock Configuration:
 *   ICLK  = 48 MHz
 *   PCLKB = 24 MHz (Clock source for AGT0, div /1)
 *   -> 24 counts = 1 microsecond.
 */

#include "hal/hal_timer.h"

#define AGT_PERIOD_COUNTS   (0x10000u)   /* 65536 counts per overflow */
#define PCLKB_MHZ           (24u)        /* 24 MHz -> 24 counts per 1 us */

static volatile uint32_t s_overflow_count = 0u;

void hal_timer_overflow_callback(timer_callback_args_t *p_args)
{
    if (p_args->event == TIMER_EVENT_CYCLE_END)
    {
        s_overflow_count++;
    }
}

void hal_timer_init(void)
{
    R_AGT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    R_AGT_Start(&g_timer0_ctrl);
}

uint32_t hal_timer_get_count_us(void)
{
    uint32_t overflow_before;
    uint32_t overflow_after;
    timer_status_t status;

    /* Double-read to prevent race condition across timer wrap */
    do
    {
        overflow_before = s_overflow_count;
        R_AGT_StatusGet(&g_timer0_ctrl, &status);
        overflow_after = s_overflow_count;
    } while (overflow_before != overflow_after);

    /* AGT counts DOWN -- elapsed counts within current period = (period - counter) */
    uint32_t counts_this_period = AGT_PERIOD_COUNTS - status.counter;
    uint64_t total_counts = ((uint64_t)overflow_after * AGT_PERIOD_COUNTS) + counts_this_period;

    return (uint32_t)(total_counts / PCLKB_MHZ);
}