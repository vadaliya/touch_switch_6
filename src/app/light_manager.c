#include "app/light_manager.h"
#include "app/state_persistence_manager.h"
#include "drivers/relay_driver.h"

static uint8_t s_light_state_mask = 0u;

void light_manager_init(void)
{
    static bool s_initialized = false;
    if (s_initialized)
    {
        return;
    }
    s_initialized = true;

    relay_driver_init();
    s_light_state_mask = 0u;
}

void light_manager_apply_mask(uint8_t mask)
{
    mask &= 0x3Fu;
    s_light_state_mask = mask;

    for (uint8_t i = 0u; i < LIGHT_SWITCH_COUNT; i++)
    {
        bool on = (mask & (uint8_t)(1u << i)) != 0u;
        relay_driver_set_light((uint8_t)(i + 1u), on);
    }
}

void light_manager_set_switch(uint8_t switch_id, bool on)
{
    if ((switch_id < 1u) || (switch_id > LIGHT_SWITCH_COUNT))
    {
        return;
    }

    relay_driver_set_light(switch_id, on);

    if (on)
    {
        s_light_state_mask |= (uint8_t) (1u << (switch_id - 1u));
    }
    else
    {
        s_light_state_mask &= (uint8_t) ~(1u << (switch_id - 1u));
    }

    state_persistence_manager_save(s_light_state_mask);
}

void light_manager_toggle_switch(uint8_t switch_id)
{
    if ((switch_id < 1u) || (switch_id > LIGHT_SWITCH_COUNT))
    {
        return;
    }

    bool current = relay_driver_get_light(switch_id);
    light_manager_set_switch(switch_id, !current);
}

bool light_manager_get_switch(uint8_t switch_id)
{
    if ((switch_id < 1u) || (switch_id > LIGHT_SWITCH_COUNT))
    {
        return false;
    }

    return relay_driver_get_light(switch_id);
}

uint8_t light_manager_get_mask(void)
{
    return relay_driver_get_light_mask();
}

void light_manager_all_on(void)
{
    light_manager_apply_mask(0x3Fu);
    state_persistence_manager_save(s_light_state_mask);
}

void light_manager_all_off(void)
{
    light_manager_apply_mask(0x00u);
    state_persistence_manager_save(s_light_state_mask);
}
