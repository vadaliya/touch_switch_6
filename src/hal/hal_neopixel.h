/*
 * hal_neopixel.h
 *
 * Bit-bang transmit for WS2812-style addressable RGB LEDs (specifically
 * IC4M-B1204RGB15CGG on this board). There's no FSP peripheral module
 * for this protocol -- it's a precise single-wire timing scheme, not
 * something r_ioport or any timer module speaks natively -- so this
 * drives the data line directly, interrupts disabled for the transmit
 * window.
 *
 * *** TIMING VALIDATED ON HARDWARE *** -- hal_neopixel.c's transmit
 * engine is a hand-cycle-counted Cortex-M23 assembly routine, verified
 * against real IC4M-B1204RGB15CGG hardware across all 3 chains (full
 * RGBW cycle + chaser test) before being ported into this file. If you
 * change the CPU clock frequency, swap MCU, or change compiler/
 * optimization flags, re-verify -- the cycle counts are hand-derived
 * for this exact toolchain/clock combination, not computed at runtime.
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
 * Sends grb_buffer out data_pin as a WS2812-style frame followed by
 * the required >200us low reset/latch period.
 *
 * @param data_pin    pin to bit-bang (must be one of the 3 pins
 *                    recognized by resolve_pin_regs() in hal_neopixel.c)
 * @param grb_buffer  led_count * 3 bytes, GRB order per LED
 * @param led_count   number of LEDs in this chain
 *
 * Disables interrupts for the duration of the bit-shifting (not the
 * reset pulse, which runs after re-enabling interrupts) -- short
 * enough for this project's chain lengths (max 5 LEDs) to have no
 * meaningful impact on other timing (20ms touch scan, etc.).
 */
void hal_neopixel_transmit(bsp_io_port_pin_t data_pin, const uint8_t *grb_buffer, uint16_t led_count);

#endif /* HAL_NEOPIXEL_H */