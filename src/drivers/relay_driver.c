/*
 * relay_driver.c
 *
 * Relay driver for 6-switch module.
 * Pin map matches FSP configuration for 6 relays + Relay Enable.
 */

#include "drivers/relay_driver.h"
#include "hal/hal_gpio.h"

#define RELAY_ENABLE_ACTIVE_HIGH   (true)

/* Inverted relay pin logic: ON = LOW (0), OFF = HIGH (1) */
#define RELAY_PIN_LEVEL_ON         (false)
#define RELAY_PIN_LEVEL_OFF        (true)

static const bsp_io_port_pin_t s_light_relay_pin[6] =
{
    FSP_RELAY_SW1,   /* P002, OUT-1 -> SW1 Relay */
    FSP_RELAY_SW2,   /* P010, OUT-2 -> SW2 Relay */
    FSP_RELAY_SW3,   /* P011, OUT-3 -> SW3 Relay */
    FSP_RELAY_SW4,   /* P012, OUT-4 -> SW4 Relay */
    FSP_RELAY_SW5,   /* P013, OUT-5 -> SW5 Relay */
    FSP_RELAY_SW6,   /* P015, OUT-6 -> SW6 Relay */
};

/* P500, physical pin 37 -> OUT Out Enable */
static const bsp_io_port_pin_t s_output_enable_pin = FSP_RELAY_EN;

static uint8_t s_light_state_mask = 0u;
static bool    s_initialized = false;

void relay_driver_init(void)
{
    if (s_initialized)
    {
        return;
    }
    s_initialized = true;

    for (uint8_t i = 0u; i < 6u; i++)
    {
        hal_gpio_write(s_light_relay_pin[i], RELAY_PIN_LEVEL_OFF);
    }
    s_light_state_mask = 0u;

    /* Assert enable only AFTER every relay pin is at its known-off
     * state -- avoids a glitch while pins are still at post-reset default. */
    relay_driver_set_output_enable(true);
}

void relay_driver_set_light(uint8_t switch_id, bool on)
{
    if ((switch_id < 1u) || (switch_id > 6u))
    {
        return;
    }

    uint8_t index = (uint8_t) (switch_id - 1u);
    hal_gpio_write(s_light_relay_pin[index], on ? RELAY_PIN_LEVEL_ON : RELAY_PIN_LEVEL_OFF);

    if (on)
    {
        s_light_state_mask |= (uint8_t) (1u << index);
    }
    else
    {
        s_light_state_mask &= (uint8_t) ~(1u << index);
    }
}

bool relay_driver_get_light(uint8_t switch_id)
{
    if ((switch_id < 1u) || (switch_id > 6u))
    {
        return false;
    }
    return (s_light_state_mask & (uint8_t) (1u << (switch_id - 1u))) != 0u;
}

uint8_t relay_driver_get_light_mask(void)
{
    return s_light_state_mask;
}

void relay_driver_set_output_enable(bool enabled)
{
    bool pin_level = RELAY_ENABLE_ACTIVE_HIGH ? enabled : !enabled;
    hal_gpio_write(s_output_enable_pin, pin_level);
}

void relay_driver_all_off(void)
{
    for (uint8_t i = 0u; i < 6u; i++)
    {
        hal_gpio_write(s_light_relay_pin[i], RELAY_PIN_LEVEL_OFF);
    }
    s_light_state_mask = 0u;
}