#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#define LIGHT_SWITCH_COUNT  (6u)

void light_manager_init(void);
void light_manager_apply_mask(uint8_t mask);
void light_manager_set_switch(uint8_t switch_id, bool on);
void light_manager_toggle_switch(uint8_t switch_id);
bool light_manager_get_switch(uint8_t switch_id);
uint8_t light_manager_get_mask(void);
void light_manager_all_on(void);
void light_manager_all_off(void);

#endif /* LIGHT_MANAGER_H */
