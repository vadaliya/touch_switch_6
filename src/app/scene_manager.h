#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#define SCENE_COUNT         (8u)
#define SCENE_MIN_OUTPUTS   (2u)
#define SCENE_MAX_OUTPUTS   (5u)    /* 6 switches: max outputs = 6 - 1 = 5 */

void    scene_manager_init(void);
void    scene_manager_reset(void);

bool    scene_manager_register_scene(uint8_t scene_index, uint8_t switch_mask, uint16_t ir_code);
bool    scene_manager_unregister_scene(uint8_t scene_index);
bool    scene_manager_execute_scene(uint8_t scene_index);
bool    scene_manager_execute_by_code(uint16_t ir_code);

uint8_t  scene_manager_get_scene_mask(uint8_t scene_index);
uint16_t scene_manager_get_scene_code(uint8_t scene_index);
bool     scene_manager_is_scene_active(uint8_t scene_index);

bool    scene_manager_is_duplicate_mask(uint8_t switch_mask, uint8_t *out_existing_index);
uint8_t scene_manager_find_slot_for_mask(uint8_t switch_mask);
uint8_t scene_manager_count_outputs(uint8_t switch_mask);

#endif /* SCENE_MANAGER_H */
