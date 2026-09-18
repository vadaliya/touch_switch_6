/*
 * hal_timer.h
 *
 * Free-running microsecond counter, built on r_agt (g_timer0). This is
 * NOT a generic "wrap every FSP timer module" layer -- the rest of this
 * project uses FreeRTOS's own tick (xTaskGetTickCount/vTaskDelay) for
 * everything coarser than ~1ms (debounce, blink timing, long-press
 * detection, etc.), which needs no dedicated hardware timer. This file
 * exists specifically because the upcoming IR driver needs
 * microsecond-resolution edge-to-edge interval measurement to decode
 * RC5 timing, which the OS tick cannot provide.
 *
 * AGT is a 16-bit down-counter -- at a configured 1us/count rate, it
 * wraps every 65.535ms. hal_timer_get_count_us() combines the current
 * counter value with a software-maintained overflow count (incremented
 * by a period-end callback) to produce a monotonically increasing
 * 32-bit microsecond value that only wraps after ~71.5 minutes --
 * ample margin for measuring RC5's few-millisecond intervals.
 *
 * *** SETUP NEEDED IN THE CONFIGURATOR ***
 * Add g_timer0 (New Stack -> Timers -> Timer, Low-Power (r_agt)),
 * Periodic mode, period = 0xFFFF (max), and set its source clock
 * division so the counter decrements at exactly 1MHz (1us/count) --
 * the exact divider depends on your configured PCLKB frequency (check
 * the Clocks tab), e.g. PCLKB=48MHz needs a /48 division if available,
 * or nearest achievable division with the resulting us-per-count
 * adjusted in AGT_COUNT_TO_US below if 1us/count isn't exactly
 * achievable on your clock configuration. Set Callback to
 * hal_timer_overflow_callback.
 */

#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include "hal_data.h"   /* FSP-generated: g_timer0 instance */
#include <stdint.h>

/**
 * hal_timer_init
 * Opens and starts g_timer0 in free-running periodic mode.
 */
void hal_timer_init(void);

/**
 * hal_timer_get_count_us
 * @return  monotonically increasing microsecond count since
 *          hal_timer_init() (wraps at UINT32_MAX us, ~71.5 minutes --
 *          callers measuring short intervals via subtraction are
 *          unaffected by the wrap as long as unsigned arithmetic is
 *          used, e.g. (uint32_t)(now - previous)).
 */
uint32_t hal_timer_get_count_us(void);

#endif /* HAL_TIMER_H */