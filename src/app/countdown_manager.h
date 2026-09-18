/*
 * countdown_manager.h
 *
 * Independent auto-off countdown timer manager for 6 Light Switches.
 * Timeouts are tracked in seconds per Tuya smart switch DP specification.
 */

#ifndef COUNTDOWN_MANAGER_H
#define COUNTDOWN_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define COUNTDOWN_LIGHT_COUNT   (6u)

/**
 * countdown_manager_init
 * Initializes all countdown timers to 0 (disabled).
 */
void countdown_manager_init(void);

/**
 * countdown_manager_set_light
 * Sets or updates auto-off countdown for a light switch (1..6).
 * @param switch_id  1..6
 * @param seconds    remaining seconds (0 = cancel)
 */
void countdown_manager_set_light(uint8_t switch_id, uint32_t seconds);

/**
 * countdown_manager_get_light
 * @param switch_id  1..6
 * @return remaining seconds, or 0 if inactive
 */
uint32_t countdown_manager_get_light(uint8_t switch_id);

/**
 * countdown_manager_cancel_light
 * Cancels auto-off countdown for a light switch (1..6).
 */
void countdown_manager_cancel_light(uint8_t switch_id);

/**
 * countdown_manager_cancel_all
 * Cancels all light auto-off timers.
 */
void countdown_manager_cancel_all(void);

/**
 * countdown_manager_tick
 * Advances active countdown timers. Call periodically.
 *
 * @param elapsed_ms  milliseconds elapsed since last call
 */
void countdown_manager_tick(uint32_t elapsed_ms);

#endif /* COUNTDOWN_MANAGER_H */
