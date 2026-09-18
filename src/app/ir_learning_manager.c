/*
 * ir_learning_manager.c
 *
 * IR learning manager for 6-switch module (S1-S6 and Scenes 1-8).
 */

#include "app/ir_learning_manager.h"
#include "app/scene_manager.h"
#include "app/backlight_manager.h"
#include "app/state_persistence_manager.h"
#include "drivers/eeprom_driver.h"
#include "drivers/rgb_led_driver.h"
#include "drivers/ir_driver.h"
#include "app/app_log.h"

#include <string.h>

#define IR_EEPROM_SWITCH_BASE       (0x000u)                                      /* 6 x 2 = 12 bytes (0x000..0x00B) */
#define IR_EEPROM_SCENE_CODE_BASE   (IR_EEPROM_SWITCH_BASE + (IR_SWITCH_COUNT * sizeof(uint16_t))) /* 0x00C (8 x 2 = 16 bytes) */
#define IR_EEPROM_SCENE_MASK_BASE   (IR_EEPROM_SCENE_CODE_BASE + (IR_SCENE_COUNT * sizeof(uint16_t))) /* 0x01C (8 x 1 = 8 bytes) */
#define IR_EEPROM_TOTAL_BYTES       (IR_EEPROM_SCENE_MASK_BASE + IR_SCENE_COUNT)  /* 36 bytes (0x024) */

static ir_learn_state_t s_state = IR_LEARN_STATE_IDLE;
static uint8_t          s_active_switch_id = 0u;
static uint8_t          s_active_scene_index = 0u;
static uint8_t          s_active_scene_mask = 0u;
static uint32_t         s_elapsed_ms = 0u;
static bool             s_learning_active = false;

static uint16_t s_switch_code[IR_SWITCH_COUNT] = { 0u, 0u, 0u, 0u, 0u, 0u };
static uint16_t s_scene_code[IR_SCENE_COUNT]   = { 0u };
static uint8_t  s_scene_mask[IR_SCENE_COUNT]   = { 0u };

static uint16_t ir_learning_manager_read_word(uint32_t offset)
{
    uint8_t bytes[2] = { 0u, 0u };
    (void) eeprom_read(offset, bytes, sizeof(bytes), 50u);
    return (uint16_t) ((uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8u));
}

static uint16_t s_active_blink_mask = 0u;
static bool     s_active_blink_set  = false;

static void ir_learning_manager_apply_led_blink_for_switch(uint8_t switch_id)
{
    rgb_color_t on_color;
    backlight_manager_get_on_color(&on_color);

    if ((switch_id >= 1u) && (switch_id <= IR_SWITCH_COUNT))
    {
        uint8_t mask = rgb_led_driver_switch_led_bit(switch_id);
        if (mask == 0u)
        {
            return;
        }

        rgb_led_driver_start_blink(mask, on_color, 500u);
        s_active_blink_mask = mask;
        s_active_blink_set  = true;
    }
}

static void ir_learning_manager_apply_led_blink_for_scene(uint8_t scene_mask)
{
    rgb_color_t on_color;
    backlight_manager_get_on_color(&on_color);

    uint8_t switch_led_mask = 0u;
    for (uint8_t i = 1u; i <= IR_SWITCH_COUNT; i++)
    {
        if ((scene_mask & (1u << (i - 1u))) != 0u)
        {
            switch_led_mask |= rgb_led_driver_switch_led_bit(i);
        }
    }

    if (switch_led_mask != 0u)
    {
        rgb_led_driver_start_blink(switch_led_mask, on_color, 500u);
        s_active_blink_mask = switch_led_mask;
        s_active_blink_set  = true;
    }
}

static void ir_learning_manager_stop_led_blink(void)
{
    if (s_active_blink_set)
    {
        rgb_led_driver_stop_blink(s_active_blink_mask);
        s_active_blink_set = false;
        s_active_blink_mask = 0u;
    }
}

static bool ir_learning_manager_persist_all(void)
{
    /* 1. Read existing backlight config from 0x100 so it isn't lost when erasing Block 0 */
    backlight_config_t cfg;
    bool has_cfg = eeprom_read(BACKLIGHT_EEPROM_OFFSET, (uint8_t *) &cfg, sizeof(cfg), 100u);

    /* Read existing Power Recovery config from 0x110 */
    uint8_t pr_buf[4];
    bool has_pr = eeprom_read(POWER_RECOVERY_EEPROM_OFFSET, pr_buf, sizeof(pr_buf), 100u);

    /* Read existing Master Switch config from 0x120 */
    uint8_t ms_buf[4];
    bool has_ms = eeprom_read(0x120u, ms_buf, sizeof(ms_buf), 100u);

    /* 2. Prepare IR code buffer */
    uint8_t ir_buf[IR_EEPROM_TOTAL_BYTES];
    memset(ir_buf, 0, sizeof(ir_buf));

    for (uint8_t i = 0u; i < IR_SWITCH_COUNT; i++)
    {
        ir_buf[IR_EEPROM_SWITCH_BASE + (i * 2u)]      = (uint8_t) (s_switch_code[i] & 0xFFu);
        ir_buf[IR_EEPROM_SWITCH_BASE + (i * 2u) + 1u] = (uint8_t) ((s_switch_code[i] >> 8u) & 0xFFu);
    }

    for (uint8_t i = 0u; i < IR_SCENE_COUNT; i++)
    {
        ir_buf[IR_EEPROM_SCENE_CODE_BASE + (i * 2u)]      = (uint8_t) (s_scene_code[i] & 0xFFu);
        ir_buf[IR_EEPROM_SCENE_CODE_BASE + (i * 2u) + 1u] = (uint8_t) ((s_scene_code[i] >> 8u) & 0xFFu);
        ir_buf[IR_EEPROM_SCENE_MASK_BASE + i]             = s_scene_mask[i];
    }

    /* 3. Erase Block 0 (1 block = 1024 bytes) */
    if (!eeprom_erase_blocking(0u, 1u, 2000u))
    {
        LOG_ERROR("IR Learning: Flash Erase Block 0 failed!");
        return false;
    }

    /* 4. Write updated IR codes & scenes to 0x000 */
    if (!eeprom_write_blocking(0u, ir_buf, sizeof(ir_buf), 1000u))
    {
        LOG_ERROR("IR Learning: Flash Write IR codes failed!");
        return false;
    }

    /* 5. Restore backlight config if it was valid */
    if (has_cfg && (cfg.magic == BACKLIGHT_MAGIC))
    {
        (void) eeprom_write_blocking(BACKLIGHT_EEPROM_OFFSET, (const uint8_t *) &cfg, sizeof(cfg), 1000u);
    }

    /* 6. Restore power recovery config if it was valid */
    if (has_pr && (pr_buf[0] == POWER_RECOVERY_MAGIC))
    {
        (void) eeprom_write_blocking(POWER_RECOVERY_EEPROM_OFFSET, pr_buf, sizeof(pr_buf), 1000u);
    }

    /* 7. Restore master switch config if it was valid */
    if (has_ms && (ms_buf[0] == 0xD4u))
    {
        (void) eeprom_write_blocking(0x120u, ms_buf, sizeof(ms_buf), 1000u);
    }

    LOG_INFO("IR Codes Saved to Flash: SW1=0x%04X SW2=0x%04X SW3=0x%04X SW4=0x%04X SW5=0x%04X SW6=0x%04X",
             s_switch_code[0], s_switch_code[1], s_switch_code[2],
             s_switch_code[3], s_switch_code[4], s_switch_code[5]);

    return true;
}

void ir_learning_manager_init(void)
{
    static const uint16_t s_default_switch_codes[IR_SWITCH_COUNT] = {
        IR_DEFAULT_CODE_SWITCH_1,
        IR_DEFAULT_CODE_SWITCH_2,
        IR_DEFAULT_CODE_SWITCH_3,
        IR_DEFAULT_CODE_SWITCH_4,
        IR_DEFAULT_CODE_SWITCH_5,
        IR_DEFAULT_CODE_SWITCH_6
    };

    for (uint8_t i = 0u; i < IR_SWITCH_COUNT; i++)
    {
        s_switch_code[i] = ir_learning_manager_read_word(IR_EEPROM_SWITCH_BASE + ((uint32_t) i * sizeof(uint16_t)));
        if ((s_switch_code[i] == 0xFFFFu) || (s_switch_code[i] == 0u))
        {
            s_switch_code[i] = s_default_switch_codes[i];
        }
    }

    scene_manager_reset();

    for (uint8_t i = 0u; i < IR_SCENE_COUNT; i++)
    {
        s_scene_code[i] = ir_learning_manager_read_word(IR_EEPROM_SCENE_CODE_BASE + ((uint32_t) i * sizeof(uint16_t)));
        if (s_scene_code[i] == 0xFFFFu) s_scene_code[i] = 0u;

        uint8_t mask_byte = 0u;
        (void) eeprom_read(IR_EEPROM_SCENE_MASK_BASE + i, &mask_byte, 1u, 50u);
        if (mask_byte == 0xFFu) mask_byte = 0u;
        s_scene_mask[i] = mask_byte;

        if ((s_scene_code[i] != 0u) && (s_scene_mask[i] != 0u))
        {
            scene_manager_register_scene(i, s_scene_mask[i], s_scene_code[i]);
        }
    }

    LOG_INFO("IR Codes Restored: SW1=0x%04X SW2=0x%04X SW3=0x%04X SW4=0x%04X SW5=0x%04X SW6=0x%04X",
             s_switch_code[0], s_switch_code[1], s_switch_code[2],
             s_switch_code[3], s_switch_code[4], s_switch_code[5]);

    ir_learning_manager_reset();
}

void ir_learning_manager_reset(void)
{
    bool was_active = s_learning_active;

    s_state = IR_LEARN_STATE_IDLE;
    s_active_switch_id = 0u;
    s_active_scene_index = 0u;
    s_active_scene_mask = 0u;
    s_elapsed_ms = 0u;
    s_learning_active = false;
    ir_learning_manager_stop_led_blink();

    if (was_active)
    {
        backlight_manager_refresh_all();
    }
}

void ir_learning_manager_handle_touch_long_press(uint8_t switch_id)
{
    if ((switch_id < 1u) || (switch_id > IR_SWITCH_COUNT))
    {
        return;
    }

    if (s_learning_active)
    {
        return;
    }

    s_state = IR_LEARN_STATE_SWITCH;
    s_active_switch_id = switch_id;
    s_elapsed_ms = 0u;
    s_learning_active = true;
    ir_learning_manager_apply_led_blink_for_switch(switch_id);
}

bool ir_learning_manager_start_scene_learning(uint8_t scene_mask)
{
    if (s_learning_active)
    {
        return false;
    }

    uint8_t output_count = scene_manager_count_outputs(scene_mask);
    if ((output_count < SCENE_MIN_OUTPUTS) || (output_count > SCENE_MAX_OUTPUTS))
    {
        LOG_WARNING("IR Scene Learn: Invalid output count (%u) for mask 0x%02X", output_count, scene_mask);
        return false;
    }

    uint8_t target_slot = scene_manager_find_slot_for_mask(scene_mask);

    s_state              = IR_LEARN_STATE_SCENE;
    s_active_scene_index = target_slot;
    s_active_scene_mask  = scene_mask;
    s_elapsed_ms         = 0u;
    s_learning_active    = true;

    LOG_INFO("IR Scene Learn: Target Scene %u for Mask 0x%02X (waiting for IR remote button...)",
             target_slot + 1u, scene_mask);

    ir_learning_manager_apply_led_blink_for_scene(scene_mask);
    return true;
}

static void remove_duplicate_code_assignment(uint16_t clean_code)
{
    if (clean_code == 0u)
    {
        return;
    }

    for (uint8_t i = 0u; i < IR_SWITCH_COUNT; i++)
    {
        if (ir_driver_strip_toggle(s_switch_code[i]) == clean_code)
        {
            LOG_INFO("IR Learn: Switch %u code 0x%04X cleared (reassigned)", i + 1u, s_switch_code[i]);
            s_switch_code[i] = 0u;
        }
    }

    for (uint8_t i = 0u; i < IR_SCENE_COUNT; i++)
    {
        if (ir_driver_strip_toggle(s_scene_code[i]) == clean_code)
        {
            LOG_INFO("IR Learn: Scene %u code 0x%04X cleared (reassigned)", i + 1u, s_scene_code[i]);
            s_scene_code[i] = 0u;
            s_scene_mask[i] = 0u;
            scene_manager_unregister_scene(i);
        }
    }
}

void ir_learning_manager_handle_ir_event(uint16_t rc5_code)
{
    if (!s_learning_active)
    {
        return;
    }

    if (s_state == IR_LEARN_STATE_SWITCH)
    {
        ir_learning_manager_store_switch_code(s_active_switch_id, rc5_code);
    }
    else if (s_state == IR_LEARN_STATE_SCENE)
    {
        ir_learning_manager_store_scene_code(s_active_scene_index, s_active_scene_mask, rc5_code);
    }

    ir_learning_manager_reset();
}

void ir_learning_manager_tick(uint32_t elapsed_ms)
{
    if (!s_learning_active)
    {
        return;
    }

    s_elapsed_ms += elapsed_ms;

    if (s_elapsed_ms >= IR_LEARN_TIMEOUT_MS)
    {
        LOG_INFO("IR Learning Mode timed out (15s elapsed)");
        ir_learning_manager_reset();
    }
}

bool ir_learning_manager_is_learning(void)
{
    return s_learning_active;
}

ir_learn_state_t ir_learning_manager_get_state(void)
{
    return s_state;
}

bool ir_learning_manager_handle_scene_ir_event(uint8_t scene_index, uint16_t rc5_code)
{
    if (!s_learning_active || (s_state != IR_LEARN_STATE_SCENE))
    {
        return false;
    }

    if (scene_index >= IR_SCENE_COUNT)
    {
        LOG_WARNING("IR Scene Learn: Invalid scene index %u", scene_index + 1u);
        ir_learning_manager_reset();
        return false;
    }

    uint8_t existing_idx = 0u;
    if (scene_manager_is_duplicate_mask(s_active_scene_mask, &existing_idx))
    {
        if (existing_idx != scene_index)
        {
            LOG_WARNING("IR Scene Learn: Switch combination 0x%02X already assigned to Scene %u! Duplicate rejected.",
                        s_active_scene_mask, existing_idx + 1u);
            ir_learning_manager_reset();
            return false;
        }
    }

    bool success = ir_learning_manager_store_scene_code(scene_index, s_active_scene_mask, rc5_code);
    ir_learning_manager_reset();
    return success;
}

bool ir_learning_manager_store_switch_code(uint8_t switch_id, uint16_t code)
{
    if ((switch_id < 1u) || (switch_id > IR_SWITCH_COUNT))
    {
        return false;
    }

    uint16_t clean_code = ir_driver_strip_toggle(code);
    if (clean_code == 0u)
    {
        return false;
    }

    remove_duplicate_code_assignment(clean_code);

    s_switch_code[switch_id - 1u] = clean_code;
    LOG_INFO("IR Learn: Switch %u assigned code 0x%04X", switch_id, clean_code);

    return ir_learning_manager_persist_all();
}

bool ir_learning_manager_store_scene_code(uint8_t scene_index, uint8_t scene_mask, uint16_t code)
{
    if (scene_index >= IR_SCENE_COUNT)
    {
        return false;
    }

    uint16_t clean_code = ir_driver_strip_toggle(code);
    if (clean_code == 0u)
    {
        return false;
    }

    remove_duplicate_code_assignment(clean_code);

    s_scene_code[scene_index] = clean_code;
    s_scene_mask[scene_index] = scene_mask;

    scene_manager_register_scene(scene_index, scene_mask, clean_code);
    LOG_INFO("IR Scene Learn: Scene %u (Mask 0x%02X) assigned code 0x%04X",
             scene_index + 1u, scene_mask, clean_code);

    return ir_learning_manager_persist_all();
}

uint16_t ir_learning_manager_get_switch_code(uint8_t switch_id)
{
    if ((switch_id < 1u) || (switch_id > IR_SWITCH_COUNT))
    {
        return 0u;
    }
    return s_switch_code[switch_id - 1u];
}

uint16_t ir_learning_manager_get_scene_code(uint8_t scene_index)
{
    if (scene_index >= IR_SCENE_COUNT)
    {
        return 0u;
    }
    return s_scene_code[scene_index];
}

uint8_t ir_learning_manager_get_scene_mask(uint8_t scene_index)
{
    if (scene_index >= IR_SCENE_COUNT)
    {
        return 0u;
    }
    return s_scene_mask[scene_index];
}