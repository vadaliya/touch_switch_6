/*
 * ir_learning_manager.h
 *
 * IR learning manager for 6-switch module (S1-S6 and Scenes 1-8).
 */

#ifndef IR_LEARNING_MANAGER_H
#define IR_LEARNING_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#define IR_LEARN_TIMEOUT_MS        (15000u)
#define IR_LEARN_TRIGGER_MS        (5000u)
#define IR_SCENE_COUNT             (8u)
#define IR_SWITCH_COUNT            (6u)

typedef enum
{
    IR_LEARN_STATE_IDLE = 0,
    IR_LEARN_STATE_SWITCH,
    IR_LEARN_STATE_SCENE,
} ir_learn_state_t;

void ir_learning_manager_init(void);
void ir_learning_manager_reset(void);

void ir_learning_manager_handle_touch_long_press(uint8_t switch_id);
bool ir_learning_manager_start_scene_learning(uint8_t scene_mask);
void ir_learning_manager_handle_ir_event(uint16_t rc5_code);
bool ir_learning_manager_handle_scene_ir_event(uint8_t scene_index, uint16_t rc5_code);
void ir_learning_manager_tick(uint32_t elapsed_ms);

bool             ir_learning_manager_is_learning(void);
ir_learn_state_t ir_learning_manager_get_state(void);

bool ir_learning_manager_store_switch_code(uint8_t switch_id, uint16_t code);
bool ir_learning_manager_store_scene_code(uint8_t scene_index, uint8_t scene_mask, uint16_t code);

uint16_t ir_learning_manager_get_switch_code(uint8_t switch_id);
uint16_t ir_learning_manager_get_scene_code(uint8_t scene_index);
uint8_t  ir_learning_manager_get_scene_mask(uint8_t scene_index);

#endif /* IR_LEARNING_MANAGER_H */
