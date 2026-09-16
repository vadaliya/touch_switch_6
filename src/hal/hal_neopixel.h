/*
 * hal_neopixel.h
 *
 * Bit-bang transmit for WS2812-style addressable RGB LEDs. There's no
 * FSP peripheral module for this protocol -- it's a precise single-
 * wire timing scheme, not something r_ioport or any timer module
 * speaks natively -- so this drives the data line directly with
 * cycle-counted delays, interrupts disabled for the transmit window.
 *
 * *** TIMING IS UNVERIFIED WITHOUT HARDWARE *** -- the delay constants
 * in hal_neopixel.c are computed from BSP_ICLK_HZ (your configured
 * system clock) against WS2812B's published timing, but compiler
 * codegen affects real-world loop timing. Verify with a logic
 * analyzer/scope once boards exist, the same way touch thresholds and
 * relay pin polarity need bench verification. If timing turns out
 * unreliable, the standard upgrade path is a GPT-timer + DMA-driven
 * implementation instead of this busy-wait approach -- flag it if
 * you hit flicker/glitching once you can test.
 */

#ifndef HAL_NEOPIXEL_H
#define HAL_NEOPIXEL_H

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * hal_neopixel_init
 * Configures the given pin as an output, driven low (idle state).
 */
void hal_neopixel_init(bsp_io_port_pin_t data_pin);

/**
 * hal_neopixel_transmit
 *
 * Sends grb_buffer out data_pin as a WS2812 frame followed by the
 * required >50us low "reset"/latch period.
 *
 * @param data_pin    pin to bit-bang
 * @param grb_buffer  led_count * 3 bytes, GRB order per LED (WS2812's
 *                    native wire order -- NOT RGB order)
 * @param led_count   number of LEDs in this chain
 *
 * Disables interrupts for the duration of the transmit (roughly
 * 30us * led_count for a 24-bit-per-LED WS2812 frame) -- short enough
 * to not meaningfully affect this project's other timing (20ms touch
 * scan, etc.) but worth knowing if you add more LEDs to either chain.
 */
void hal_neopixel_transmit(bsp_io_port_pin_t data_pin, const uint8_t *grb_buffer, uint16_t led_count);

#endif /* HAL_NEOPIXEL_H */
