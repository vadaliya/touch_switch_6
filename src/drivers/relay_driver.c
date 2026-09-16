/*
 * relay_driver.c
 *
 * See relay_driver.h. Pin map below matches the real PCB pinout
 * you provided (touch_switch_test_project).
 */

#include "drivers/relay_driver.h"
#include "hal/hal_gpio.h"

/* TODO: confirm against the relay driver IC's datasheet. true = this
 * code assumes driving P500 HIGH enables the relay outputs. Flip to
 * false if the IC's Output Enable is active-low instead. */
#define RELAY_OE_ACTIVE_HIGH   (true)

static const bsp_io_port_pin_t s_light_relay_pin[6] =
{
    BSP_IO_PORT_00_PIN_02,   /* P002, physical pin 46 -> OUT-1 -> Light1 */
    BSP_IO_PORT_00_PIN_10,   /* P010, physical pin 45 -> OUT-2 -> Light2 */
    BSP_IO_PORT_00_PIN_11,   /* P011, physical pin 44 -> OUT-3 -> Light3 */
    BSP_IO_PORT_00_PIN_12,   /* P012, physical pin 41 -> OUT-4 -> Light4 */
    BSP_IO_PORT_00_PIN_13,   /* P013, physical pin 40 -> OUT-5 -> Light5 */
    BSP_IO_PORT_00_PIN_15,   /* P015, physical pin 38 -> OUT-6 -> Light6 */
};

static const bsp_io_port_pin_t s_output_enable_pin = BSP_IO_PORT_05_PIN_00; /* P500, pin 37 */

static uint8_t s_light_state_mask = 0u;

void relay_driver_init(void)
{
    for (uint8_t i = 0u; i < 6u; i++)
    {
        hal_gpio_write(s_light_relay_pin[i], false);
    }
    s_light_state_mask = 0u;

    /* Assert Output Enable only AFTER every relay pin is already
     * driven to its known-off state -- avoids the driver IC briefly
     * enabling outputs while pin states are still at their
     * post-reset default. */
    relay_driver_set_output_enable(true);
}

void relay_driver_set_light(uint8_t switch_id, bool on)
{
    if ((switch_id < 1u) || (switch_id > 6u))
    {
        return;
    }

    uint8_t index = (uint8_t) (switch_id - 1u);

    hal_gpio_write(s_light_relay_pin[index], on);

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
    bool pin_level = RELAY_OE_ACTIVE_HIGH ? enabled : !enabled;
    hal_gpio_write(s_output_enable_pin, pin_level);
}

void relay_driver_set_fan_speed(fan_speed_t speed)
{
    (void) speed;
#if RELAY_DRIVER_HAS_FAN_HARDWARE
#error "Fan relay hardware exists but relay_driver.c wasn't updated -- add the real pin map and truth-table logic here (see chat history for the original 3-relay truth table)."
#endif
    /* No fan relay hardware on this board -- intentional no-op. */
}

fan_speed_t relay_driver_get_fan_speed(void)
{
#if RELAY_DRIVER_HAS_FAN_HARDWARE
#error "Fan relay hardware exists but relay_driver.c wasn't updated."
#endif
    return FAN_SPEED_OFF; /* always, on this board */
}

void relay_driver_all_off(void)
{
    for (uint8_t i = 0u; i < 6u; i++)
    {
        hal_gpio_write(s_light_relay_pin[i], false);
    }
    s_light_state_mask = 0u;
}
