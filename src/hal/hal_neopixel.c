/*
 * hal_neopixel.c
 *
 * See hal_neopixel.h for the "unverified without hardware" caveat --
 * read that before assuming this is production-ready timing.
 *
 * Uses hal_gpio_write() (this project's existing GPIO wrapper) for
 * each bit rather than raw PFS register access, for consistency with
 * the rest of the codebase. NOTE: R_IOPORT_PinWrite()'s function-call
 * and parameter-checking overhead eats into the already-tight WS2812
 * bit budget (~1.25us/bit). If bench testing shows glitchy/flickering
 * LEDs, the first thing to try is switching send_bit() below to direct
 * PFS register writes (R_PFS->PORT[n].PIN[m].PmnPFS) instead of
 * hal_gpio_write(), to cut per-bit overhead.
 */

#include "hal/hal_neopixel.h"
#include "hal/hal_gpio.h"

/* WS2812B timing (nanoseconds), used to compute cycle counts below.
 * TODO: confirm your actual LED part number -- SK6812/WS2812B/etc.
 * have slightly different published timings; these are WS2812B's.
 */
#define T0H_NS   (400u)
#define T0L_NS   (850u)
#define T1H_NS   (800u)
#define T1L_NS   (450u)
#define RESET_US (60u)   /* >50us low required to latch */

/* Busy-wait for approximately the given number of CPU cycles. Kept as
 * a simple volatile-counted loop rather than inline assembly for
 * portability -- calibrate/verify against a scope once hardware
 * exists; compiler optimization level affects real loop overhead.
 */
static inline void delay_cycles(uint32_t cycles)
{
    /* Rough loop overhead assumed ~3-4 cycles/iteration on Cortex-M23
     * at -O2; TODO verify. Divisor below is a starting estimate, not
     * a measured value. */
    volatile uint32_t count = cycles / 4u;
    while (count--)
    {
        __NOP();
    }
}

static uint32_t ns_to_cycles(uint32_t ns)
{
    /* SystemCoreClock is the CMSIS-standard global FSP keeps updated
     * to the actual configured ICLK frequency -- using it here rather
     * than a hardcoded MHz value means this doesn't silently break if
     * the Clocks tab configuration changes later. */
    return (uint32_t) (((uint64_t) ns * SystemCoreClock) / 1000000000ull);
}

static void send_bit(bsp_io_port_pin_t data_pin, bool bit_value)
{
    if (bit_value)
    {
        hal_gpio_write(data_pin, true);
        delay_cycles(ns_to_cycles(T1H_NS));
        hal_gpio_write(data_pin, false);
        delay_cycles(ns_to_cycles(T1L_NS));
    }
    else
    {
        hal_gpio_write(data_pin, true);
        delay_cycles(ns_to_cycles(T0H_NS));
        hal_gpio_write(data_pin, false);
        delay_cycles(ns_to_cycles(T0L_NS));
    }
}

void hal_neopixel_init(bsp_io_port_pin_t data_pin)
{
    hal_gpio_write(data_pin, false);
}

void hal_neopixel_transmit(bsp_io_port_pin_t data_pin, const uint8_t *grb_buffer, uint16_t led_count)
{
    uint32_t total_bytes = (uint32_t) led_count * 3u;

    /* Interrupts disabled for the transmit window -- WS2812 timing
     * has no tolerance for an ISR stealing the CPU mid-bit. Kept as
     * short as possible (only wraps the actual bit-shifting, not the
     * setup/reset delay below).
     */
    taskENTER_CRITICAL();

    for (uint32_t byte_idx = 0u; byte_idx < total_bytes; byte_idx++)
    {
        uint8_t byte = grb_buffer[byte_idx];

        for (int8_t bit = 7; bit >= 0; bit--)
        {
            send_bit(data_pin, (byte & (1u << bit)) != 0u);
        }
    }

    taskEXIT_CRITICAL();

    /* Reset/latch period -- interrupts fine to re-enable during this,
     * it's just a long low pulse. */
    hal_gpio_write(data_pin, false);

    for (uint32_t us = 0u; us < RESET_US; us++)
    {
        delay_cycles(ns_to_cycles(1000u));
    }
}
