/*
 * hal_gpio.h
 *
 * Thin wrapper over FSP's r_ioport driver. Pure "can I talk to this
 * pin" -- no product logic. Relay driver, and later any other GPIO
 * consumer, calls through here rather than touching R_IOPORT_* APIs
 * directly, so a future port to a different IOPORT setup (or a mock
 * for host-side unit testing) only touches this one file.
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include "hal_data.h"   /* FSP-generated: bsp_io_port_pin_t, g_ioport instance */
#include <stdbool.h>

/**
 * hal_gpio_write
 *
 * @param pin    FSP pin identifier, e.g. BSP_IO_PORT_04_PIN_00
 * @param level  true = drive high, false = drive low
 */
void hal_gpio_write(bsp_io_port_pin_t pin, bool level);

/**
 * hal_gpio_read
 *
 * Reads back the pin's current logic level. For relay outputs this is
 * mainly useful for a power-on self-test or diagnostic read-back path;
 * normal operation should track state in the driver layer instead of
 * re-reading pins, since relay driver stages are write-only in most
 * board designs (no feedback line).
 */
bool hal_gpio_read(bsp_io_port_pin_t pin);

#endif /* HAL_GPIO_H */