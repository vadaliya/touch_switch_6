/*
 * hal_ir_capture.h
 *
 * Hardware edge-timestamping for the IR receiver on P103 via GPT5
 * input capture (GTIOC5A). Replaces software polling (ir_driver.c's
 * old read_ir_pin() loop) -- the timestamp is latched by hardware the
 * instant an edge occurs, so this is immune to touch_thread's ~2.5-
 * 3.5ms CTSU preemption, which was corrupting fast taps under the old
 * polling design.
 *
 * GPT5 free-runs at 1MHz (1 count = 1us) -- see hal_ir_capture.c for
 * the exact Configurator setup this assumes.
 */

#ifndef HAL_IR_CAPTURE_H
#define HAL_IR_CAPTURE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint16_t duration_us; /* how long the PREVIOUS level lasted, ending at this edge (max 65.5ms) */
    uint8_t  level;       /* the level that lasted duration_us (matches ir_pulse_t's shape) */
} ir_edge_t;

/**
 * hal_ir_capture_init
 * Opens g_timer_ir_capture and enables both capture channels.
 */
void hal_ir_capture_init(void);

/**
 * hal_ir_capture_pop_edge
 *
 * Non-blocking. Pops the oldest buffered edge into *out_edge.
 * @return  true if an edge was popped, false if the ring buffer is empty.
 */
bool hal_ir_capture_pop_edge(ir_edge_t *out_edge);

#endif /* HAL_IR_CAPTURE_H */