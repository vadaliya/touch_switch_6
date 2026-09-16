/*
 * hal_gpio.c
 *
 * See hal_gpio.h. Assumes g_ioport has already been opened via
 * R_IOPORT_Open() -- this happens once in R_BSP_WarmStart()'s
 * BSP_WARM_START_POST_C block (FSP-generated), well before any
 * driver or thread code runs.
 */

#include "hal/hal_gpio.h"

void hal_gpio_write(bsp_io_port_pin_t pin, bool level)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, pin, level ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

bool hal_gpio_read(bsp_io_port_pin_t pin)
{
    bsp_io_level_t level = BSP_IO_LEVEL_LOW;

    R_IOPORT_PinRead(&g_ioport_ctrl, pin, &level);

    return (level == BSP_IO_LEVEL_HIGH);
}