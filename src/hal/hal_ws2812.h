/*
 * hal_ws2812.h
 *
 * Bit-bang transmitter for WS2812B-compatible addressable RGB LEDs.
 * No FSP peripheral module fits this protocol directly (single-wire,
 * sub-microsecond timing) -- unlike every other HAL wrapper in this
 * project, this one drives raw GPIO with cycle-counted delays instead
 * of wrapping an FSP driver.
 *
 * *** TIMING IS UNVERIFIED -- REQUIRES HARDWARE + SCOPE TO CONFIRM ***
 * The delay constants in hal_ws2812.c are computed from BSP_ICLK_HZ
 * (your configured system clock) and WS2812B's datasheet timing, but
 * compiler-optimization-dependent busy-loops are inherently fragile.
 * First bring-up on real hardware should verify T0H/T1H/T0L/T1L on an
 * oscilloscope before trusting LED color accuracy.
 *
 * Assumed chip: WS2812B. If your actual LEDs are SK6812 or another
 * variant, timing constants and/or color byte order (this assumes
 * GRB, which WS2812B uses) may need adjusting.
 */

#ifndef HAL_WS2812_H
#define HAL_WS2812_H

#include "hal_data.h"
#include <stdint.h>

/**
 * hal_ws2812_transmit
 *
 * Bit-bangs led_count pixels out on the given pin. Disables interrupts
 * for the duration of the transmission (typ. ~30us per LED, so ~330us
 * for the full 11-LED backlight chain) to avoid timing jitter from
 * FreeRTOS tick/other ISRs corrupting the WS2812 bit pattern -- this is
 * a short enough window not to meaningfully affect scheduling latency
 * elsewhere in the system, but it IS a real (if brief) latency spike;
 * keep it in mind if you ever see occasional missed deadlines on other
 * time-critical threads.
 *
 * @param pin        GPIO pin wired to this chain's DIN
 * @param grb_bytes  led_count * 3 bytes, in G,R,B,G,R,B... order per LED
 * @param led_count  number of LEDs in this chain
 */
void hal_ws2812_transmit(bsp_io_port_pin_t pin, const uint8_t *grb_bytes, uint16_t led_count);

#endif /* HAL_WS2812_H */
