#include "app/scene_manager.h"
#include "app/light_manager.h"
#include "app/state_persistence_manager.h"
#include "app/backlight_manager.h"
#include "app/app_log.h"
#include "drivers/ir_driver.h"

#include <string.h>

typedef struct
{
    uint8_t  switch_mask; /* bit 0..5: SW1..SW6 */
    uint16_t code;        /* 11-bit toggle-stripped RC5 code */
    bool     active;
} scene_record_t;

static scene_record_t s_scene[SCENE_COUNT];

uint8_t scene_manager_count_outputs(uint8_t mask)
{
    uint8_t count = 0u;
    for (uint8_t i = 0u; i < LIGHT_SWITCH_COUNT; i++)
    {
        if ((mask & (1u << i)) != 0u)
        {
            count++;
        }
    }
    return count;
}

void scene_manager_init(void)
{
    /* s_scene is populated by ir_learning_manager_init from EEPROM */
}

void scene_manager_reset(void)
{
    memset(s_scene, 0, sizeof(s_scene));
}

bool scene_manager_is_duplicate_mask(uint8_t switch_mask, uint8_t *out_existing_index)
{
    for (uint8_t i = 0u; i < SCENE_COUNT; i++)
    {
        if (s_scene[i].active && (s_scene[i].switch_mask == switch_mask))
        {
            if (out_existing_index != NULL)
            {
                *out_existing_index = i;
            }
            return true;
        }
    }
    return false;
}

uint8_t scene_manager_find_slot_for_mask(uint8_t switch_mask)
{
    /* 1. Check if same mask is already registered -> update that scene */
    uint8_t existing_idx = 0u;
    if (scene_manager_is_duplicate_mask(switch_mask, &existing_idx))
    {
        return existing_idx;
    }

    /* 2. Find first inactive slot */
    for (uint8_t i = 0u; i < SCENE_COUNT; i++)
    {
        if (!s_scene[i].active)
        {
            return i;
        }
    }

    /* 3. If all full, overwrite slot 0 */
    return 0u;
}

bool scene_manager_register_scene(uint8_t scene_index, uint8_t switch_mask, uint16_t ir_code)
{
    if (scene_index >= SCENE_COUNT)
    {
        return false;
    }

    uint8_t output_count = scene_manager_count_outputs(switch_mask);
    if ((output_count < SCENE_MIN_OUTPUTS) || (output_count > SCENE_MAX_OUTPUTS))
    {
        return false;
    }

    /* Scene uniqueness: if another scene has this same ir_code, clear it */
    uint16_t clean_code = ir_driver_strip_toggle(ir_code);
    if (clean_code != 0u)
    {
        for (uint8_t i = 0u; i < SCENE_COUNT; i++)
        {
            if ((i != scene_index) && (s_scene[i].code == clean_code))
            {
                LOG_INFO("Scene %u IR code 0x%04X cleared (reassigned to Scene %u)",
                         i + 1u, clean_code, scene_index + 1u);
                s_scene[i].code = 0u;
                s_scene[i].switch_mask = 0u;
                s_scene[i].active = false;
            }
        }
    }

    s_scene[scene_index].switch_mask = switch_mask;
    s_scene[scene_index].code        = clean_code;
    s_scene[scene_index].active      = (clean_code != 0u);
    return true;
}

bool scene_manager_unregister_scene(uint8_t scene_index)
{
    if (scene_index >= SCENE_COUNT)
    {
        return false;
    }

    s_scene[scene_index].switch_mask = 0u;
    s_scene[scene_index].code        = 0u;
    s_scene[scene_index].active      = false;
    return true;
}

bool scene_manager_execute_scene(uint8_t scene_index)
{
    if ((scene_index >= SCENE_COUNT) || !s_scene[scene_index].active)
    {
        return false;
    }

    uint8_t mask = s_scene[scene_index].switch_mask;
    LOG_INFO("Executing Scene %u: Mask=0x%02X", scene_index + 1u, mask);

    /* 1. Set Light Switches 1..6 */
    for (uint8_t i = 0u; i < LIGHT_SWITCH_COUNT; i++)
    {
        bool include = ((mask & (1u << i)) != 0u);
        light_manager_set_switch((uint8_t) (i + 1u), include);
    }

    /* 2. Persist new state to Block 1 append-log */
    state_persistence_manager_save(light_manager_get_mask());

    /* 3. Update LEDs */
    backlight_manager_refresh_all();
    return true;
}

bool scene_manager_execute_by_code(uint16_t ir_code)
{
    uint16_t clean_code = ir_driver_strip_toggle(ir_code);
    if (clean_code == 0u)
    {
        return false;
    }

    for (uint8_t i = 0u; i < SCENE_COUNT; i++)
    {
        if (s_scene[i].active && (s_scene[i].code == clean_code))
        {
            LOG_INFO("IR Match: Scene %u (Code=0x%04X)", i + 1u, ir_code);
            return scene_manager_execute_scene(i);
        }
    }
    return false;
}

uint8_t scene_manager_get_scene_mask(uint8_t scene_index)
{
    if (scene_index >= SCENE_COUNT)
    {
        return 0u;
    }
    return s_scene[scene_index].switch_mask;
}

uint16_t scene_manager_get_scene_code(uint8_t scene_index)
{
    if (scene_index >= SCENE_COUNT)
    {
        return 0u;
    }
    return s_scene[scene_index].code;
}

bool scene_manager_is_scene_active(uint8_t scene_index)
{
    if (scene_index >= SCENE_COUNT)
    {
        return false;
    }
    return s_scene[scene_index].active;
}
