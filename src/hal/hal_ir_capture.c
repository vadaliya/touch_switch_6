/*
 * hal_ir_capture.c
 * See hal_ir_capture.h.
 *
 * *** CLOCK NOTE: PCLKD = 48MHz on this board. RA's GPT source clock
 * divider only offers power-of-2 options (/1, /2, /4, /8, /16, /32,
 * /64, /256, /1024) -- there is NO /48, so a clean "1 count = 1us"
 * mapping via clock division alone isn't achievable. Fix: configure
 * the GPT source clock division to /1 (count at the full 48MHz) and
 * convert to microseconds in software via integer division by 48
 * below -- exact (48 counts = 1us precisely), same pattern hal_timer.c
 * already uses for AGT0 in this project. ***
 *
 * *** VERIFY IN CONFIGURATOR ***: g_timer_ir_capture (GPT5, GTIOC5A/
 * P103) must have Source Clock Division = /1 to match this file. If
 * you already configured a different division before reading this,
 * either change it to /1, or change GPT_COUNTS_PER_US below to match
 * whatever you actually set (48 / division_factor).
 */

#include "hal/hal_ir_capture.h"
#include "hal/hal_timer.h"
#include "hal_data.h"
#include "app/app_common.h"   /* g_ir_capture_semaphore */

#define IR_EDGE_RING_SIZE     (32u)

static volatile ir_edge_t s_ring[IR_EDGE_RING_SIZE];
static volatile uint32_t  s_ring_head = 0u; /* ISR-owned */
static volatile uint32_t  s_ring_tail = 0u; /* ir_driver-owned */

static volatile uint32_t s_last_edge_us   = 0u;
static volatile bool     s_last_level     = true; /* idle HIGH per LF0038KAHA */

void ir_capture_callback(timer_callback_args_t *p_args)
{
    if ((p_args->event != TIMER_EVENT_CAPTURE_A) && (p_args->event != TIMER_EVENT_CAPTURE_B))
    {
        return;
    }

    uint32_t now_us      = hal_timer_get_count_us();
    uint32_t duration_us = now_us - s_last_edge_us;
    if (duration_us > 0xFFFFu)
    {
        duration_us = 0xFFFFu;
    }

    uint32_t next_head = (s_ring_head + 1u) % IR_EDGE_RING_SIZE;
    if (next_head != s_ring_tail) /* drop silently if full -- better than corrupting memory */
    {
        s_ring[s_ring_head].duration_us = (uint16_t) duration_us;
        s_ring[s_ring_head].level       = (uint8_t) (s_last_level ? 1u : 0u); /* the level that JUST ENDED */
        s_ring_head = next_head;
    }

    s_last_level   = !s_last_level;
    s_last_edge_us = now_us;

    BaseType_t higher_priority_task_woken = pdFALSE;
    xSemaphoreGiveFromISR(g_ir_capture_semaphore, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

void hal_ir_capture_init(void)
{
    hal_timer_init();

    R_GPT_Open(&g_timer_ir_capture_ctrl, &g_timer_ir_capture_cfg);
    R_GPT_Enable(&g_timer_ir_capture_ctrl);
    R_GPT_Start(&g_timer_ir_capture_ctrl);

    s_ring_head    = 0u;
    s_ring_tail    = 0u;
    s_last_level   = true;
    s_last_edge_us = hal_timer_get_count_us();
}

bool hal_ir_capture_pop_edge(ir_edge_t *out_edge)
{
    if (s_ring_tail == s_ring_head)
    {
        return false; /* empty */
    }

    out_edge->duration_us = s_ring[s_ring_tail].duration_us;
    out_edge->level       = s_ring[s_ring_tail].level;
    s_ring_tail = (s_ring_tail + 1u) % IR_EDGE_RING_SIZE;

    return true;
}