/*
 * touch_driver.h
 *
 * Touch driver for 6-switch module (S1-S6, no fan, no slider).
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
 * state machine across S1-S6, and posts touch_event_t items to
 * g_touch_event_queue for each qualifying transition.
 */
void touch_driver_scan(void);

#endif /* TOUCH_DRIVER_H */