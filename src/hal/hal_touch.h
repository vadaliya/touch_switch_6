/*
 * hal_touch.h
 *
 * Thin wrapper over FSP's Touch middleware (rm_touch), which sits on
 * top of r_ctsu and handles debounce/threshold/calibration for us --
 * configured via the QE for Capacitive Touch tool (Renesas Views ->
 * Renesas QE -> CapTouch Main QE), NOT via simple module Properties.
 *
 * REPLACES the earlier hal_ctsu.h/.c, which incorrectly assumed raw
 * r_ctsu exposed a simple Callback property like most other FSP
 * modules -- it doesn't; rm_touch does.
 *
 * Element order (bit position in the button status bitmask) MUST
 * match the order buttons were added in the QE CapTouch wizard --
 * see touch_driver.h's touch_element_t for the expected order
 * (S1..S6, F1, then 4 slider zones).
 */

#ifndef HAL_TOUCH_H
#define HAL_TOUCH_H

#include "hal_data.h"   /* FSP-generated: g_touch0 instance (name set during
                          * QE CapTouch wizard setup) */
#include <stdint.h>
#include <stdbool.h>

/**
 * hal_touch_init
 *
 * Opens g_touch0. The QE-generated config already has its callback
 * wired (set in the rm_touch module's Properties -> Callback field,
 * pointing at touch_scan_complete_callback() in hal_touch.c) --
 * nothing else to register here.
 */
void hal_touch_init(void);

/**
 * hal_touch_scan_and_wait
 *
 * Starts a scan across all configured elements and blocks (up to
 * timeout_ms) until rm_touch reports the measurement update is ready.
 *
 * @return  true if the scan completed within timeout_ms, false on
 *          timeout (caller should treat this as "no new data this
 *          iteration", not a hard error).
 */
bool hal_touch_scan_and_wait(uint32_t timeout_ms);

/**
 * hal_touch_get_button_status
 *
 * @return  bit i = 1 if element i is currently touched, per
 *          RM_TOUCH_DataGet()'s already-debounced/thresholded result
 *          (calibrated via QE -- no additional software threshold
 *          needed on top of this). Bit order matches the order
 *          elements were added in the QE wizard.
 */
uint64_t hal_touch_get_button_status(void);

#endif /* HAL_TOUCH_H */