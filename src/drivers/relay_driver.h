/*
 * relay_driver.h
 *
 * UPDATED to match the real PCB pinout (touch_switch_test_project):
 * only 6 relay outputs exist (OUT-1..OUT-6, mapped to Light1-6) plus
 * a shared Output Enable line (P500) -- there is NO fan relay hardware
 * on this board revision, unlike the original 9-relay spec.
 *
 * The fan-speed API is KEPT (relay_driver_set_fan_speed/get_fan_speed)
 * so any app-layer code written against the full spec (fan_manager,
 * etc.) still links -- it just no-ops on this hardware. See the
 * RELAY_DRIVER_HAS_FAN_HARDWARE guard below; flip it if a future PCB
 * revision adds real fan relays.
 */

#ifndef RELAY_DRIVER_H
#define RELAY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/* Set to 1 once a PCB revision with real fan relay hardware exists.
 * While 0, relay_driver_set_fan_speed()/get_fan_speed() are safe no-ops
 * (always report FAN_SPEED_OFF) rather than touching nonexistent pins.
 */
#define RELAY_DRIVER_HAS_FAN_HARDWARE   (0)

typedef enum
{
    FAN_SPEED_OFF = 0,
    FAN_SPEED_1   = 1,
    FAN_SPEED_2   = 2,
    FAN_SPEED_3   = 3,
    FAN_SPEED_4   = 4,
} fan_speed_t;

/**
 * relay_driver_init
 *
 * Configures the 6 relay pins + Output Enable pin as outputs, drives
 * all 6 relays OFF, THEN asserts Output Enable -- deliberately in that
 * order, so the driver IC never briefly enables outputs while their
 * state is undefined at power-up (avoids a relay-chatter glitch).
 */
void relay_driver_init(void);

/**
 * relay_driver_set_light
 *
 * @param switch_id  1-6, corresponding to S1-S6 / OUT-1-OUT-6
 * @param on         true = energize relay (light ON), false = de-energize
 *
 * Out-of-range switch_id is silently ignored -- see relay_driver.c.
 */
void relay_driver_set_light(uint8_t switch_id, bool on);

/**
 * relay_driver_get_light
 *
 * @param switch_id  1-6
 * @return           current commanded state (internal tracking, not a
 *                    hardware read-back). Returns false if out of range.
 */
bool relay_driver_get_light(uint8_t switch_id);

/**
 * relay_driver_get_light_mask
 * @return  bit0..bit5 = S1..S6 current state (1 = ON)
 */
uint8_t relay_driver_get_light_mask(void);

/**
 * relay_driver_set_output_enable
 *
 * Drives P500. Polarity is a TODO -- confirm from the relay driver
 * IC's datasheet and flip RELAY_OE_ACTIVE_HIGH in relay_driver.c if
 * needed. Currently assumed active-high (true = outputs enabled).
 */
void relay_driver_set_output_enable(bool enabled);

/**
 * relay_driver_set_fan_speed / relay_driver_get_fan_speed
 *
 * NO-OP on this hardware -- see RELAY_DRIVER_HAS_FAN_HARDWARE above.
 * Kept in the API so app-layer code written against the full spec
 * still compiles/links against this board's driver.
 */
void relay_driver_set_fan_speed(fan_speed_t speed);
fan_speed_t relay_driver_get_fan_speed(void);

/**
 * relay_driver_all_off
 * De-energizes all 6 light relays. Does not change Output Enable state.
 */
void relay_driver_all_off(void);

#endif /* RELAY_DRIVER_H */
