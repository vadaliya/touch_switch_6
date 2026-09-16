/*
 * hal_ws2812.c
 *
 * See hal_ws2812.h for the important caveats. WS2812B timing (from
 * datasheet, nominal values):
 *   T0H ~0.4us   T0L ~0.85us   (total ~1.25us per bit)
 *   T1H ~0.8us   T1L ~0.45us
 *   Reset/latch: >50us line-low (using 60us here for margin)
 *
 * Cortex-M23 (ARMv8-M Baseline) has no DWT cycle counter, unlike
 * mainline Cortex-M cores -- so this uses a simple calibrated busy-loop
 * rather than polling a hardware cycle count. LOOP_CYCLES_PER_ITERATION
 * is a rough estimate and WILL need adjusting against a scope; if the
 * busy-loop proves too imprecise/optimization-sensitive in practice,
 * the more robust fix is repurposing a free-running GPT/AGT counter
 * and polling its count register instead of counting loop iterations.
 */

#include "hal/hal_ws2812.h"
#include "hal/hal_gpio.h"
#include "../../src/bsp/cmsis/Device/RENESAS/Include/system.h"

/* SystemCoreClock is the CMSIS-standard global FSP keeps updated to
 * the actual configured ICLK frequency. This replaces the undefined
 * BSP_ICLK_HZ macro in this workspace.
 */
#define CYCLES_PER_US                  ((SystemCoreClock + 500000u) / 1000000u)

/* TODO: CALIBRATE -- rough estimate of CPU cycles consumed per
 * iteration of the busy-loop below at typical -O2 optimization. Use a
 * scope on the DIN pin to measure actual T0H/T1H and adjust this
 * divisor (and/or add/remove NOPs in delay_cycles) until measured
 * timing matches the WS2812B datasheet within its tolerance.
 */
#define LOOP_CYCLES_PER_ITERATION      (4u)

#define T1H_CYCLES   ((CYCLES_PER_US * 8u) / 10u)   /* 0.8us */
#define T0H_CYCLES   ((CYCLES_PER_US * 4u) / 10u)   /* 0.4us */
#define BIT_PERIOD_CYCLES  (CYCLES_PER_US + (CYCLES_PER_US / 4u)) /* ~1.25us */

static void delay_cycles(uint32_t cycles)
{
    uint32_t iterations = cycles / LOOP_CYCLES_PER_ITERATION;

    for (volatile uint32_t i = 0u; i < iterations; i++)
    {
        __NOP();
    }
}

static void send_bit(bsp_io_port_pin_t pin, bool bit_is_one)
{
    uint32_t high_cycles = bit_is_one ? T1H_CYCLES : T0H_CYCLES;

    hal_gpio_write(pin, true);
    delay_cycles(high_cycles);

    hal_gpio_write(pin, false);
    delay_cycles(BIT_PERIOD_CYCLES - high_cycles);
}

void hal_ws2812_transmit(bsp_io_port_pin_t pin, const uint8_t *grb_bytes, uint16_t led_count)
{
    uint32_t total_bytes = (uint32_t) led_count * 3u;

    taskENTER_CRITICAL(); /* suppress FreeRTOS tick/ISR jitter during the
                            * timing-critical bit stream -- see header
                            * comment for why this is an acceptably
                            * short window. */

    for (uint32_t byte_idx = 0u; byte_idx < total_bytes; byte_idx++)
    {
        uint8_t byte = grb_bytes[byte_idx];

        for (int8_t bit = 7; bit >= 0; bit--)
        {
            send_bit(pin, (byte & (uint8_t) (1u << bit)) != 0u);
        }
    }

    taskEXIT_CRITICAL();

    /* Latch/reset: hold line low for >50us so the LEDs commit the
     * shifted-in data. Fine to do this outside the critical section --
     * timing here is a minimum, not a precise pulse.
     */
    hal_gpio_write(pin, false);
    delay_cycles(CYCLES_PER_US * 60u);
}
