/*
 * touch_driver.h
 *
 * UPDATED to match the real PCB pinout: only 6 touch elements exist
 * (SW-1..SW-6 -> S1..S6). NO F1 button and NO slider on this board
 * revision -- unlike the original spec's 7-button + 4-step-slider
 * design. Kept the driver's overall shape (rm_touch + long-press
 * timing) since that logic is unaffected, just fewer elements.
 *
 * ACTION NEEDED IN QE: your existing CapTouch config has 11 elements
 * (S1-S6, F1, step1-step4). Remove the F1 and 4 step elements, leaving
 * exactly 6, in this pin order (element index = bit position in
 * hal_touch_get_button_status()'s return value):
 *
 *   Index 0 (SW-1): P001, physical pin 47
 *   Index 1 (SW-2): P101, physical pin 35
 *   Index 2 (SW-3): P000, physical pin 48
 *   Index 3 (SW-4): P103, physical pin 33
 *   Index 4 (SW-5): P409, physical pin 10
 *   Index 5 (SW-6): P301, physical pin 23
 */

#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    TOUCH_ELEM_S1 = 0,
    TOUCH_ELEM_S2,
    TOUCH_ELEM_S3,
    TOUCH_ELEM_S4,
    TOUCH_ELEM_S5,
    TOUCH_ELEM_S6,
    TOUCH_ELEM_COUNT
} touch_element_t;

/**
 * touch_driver_init
 * Opens the touch peripheral (via hal_touch_init()). Call once before
 * the first touch_driver_scan().
 */
void touch_driver_init(void);

/**
 * touch_driver_scan
 * One full cycle: triggers a scan, waits for it, runs the long-press
 * state machine across all 6 elements, and posts a touch_event_t to
 * g_touch_event_queue for each qualifying transition. No slider
 * processing on this board -- TOUCH_EVT_SLIDER_UP/DOWN are simply
 * never emitted (left defined in app_common.h for forward
 * compatibility with a future PCB revision that adds one).
 */
void touch_driver_scan(void);

#endif /* TOUCH_DRIVER_H */
