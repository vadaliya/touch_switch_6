/*
 * countdown_manager.c
 *
 * Implements countdown timers for 6 light switches.
 */

#include "app/countdown_manager.h"
#include "app/light_manager.h"
#include "app/backlight_manager.h"
#include "app/app_common.h"
#include "app/app_log.h"
#include "app/tuya_dp_handlers.h"

static uint32_t s_light_timers[COUNTDOWN_LIGHT_COUNT] = { 0u, 0u, 0u, 0u, 0u, 0u };
static uint32_t s_ms_accumulator = 0u;

void countdown_manager_init(void)
{
    for (uint8_t i = 0u; i < COUNTDOWN_LIGHT_COUNT; i++)
    {
        s_light_timers[i] = 0u;
    }
    s_ms_accumulator = 0u;
}

void countdown_manager_set_light(uint8_t switch_id, uint32_t seconds)
{
    if ((switch_id < 1u) || (switch_id > COUNTDOWN_LIGHT_COUNT))
    {
        return;
    }

    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        s_light_timers[switch_id - 1u] = seconds;
        xSemaphoreGive(g_config_mutex);
    }
    LOG_INFO("Countdown: Light SW%u set to %lu seconds", switch_id, (unsigned long) seconds);
}

uint32_t countdown_manager_get_light(uint8_t switch_id)
{
    if ((switch_id < 1u) || (switch_id > COUNTDOWN_LIGHT_COUNT))
    {
        return 0u;
    }

    uint32_t val = 0u;
    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        val = s_light_timers[switch_id - 1u];
        xSemaphoreGive(g_config_mutex);
    }
    return val;
}

void countdown_manager_cancel_light(uint8_t switch_id)
{
    countdown_manager_set_light(switch_id, 0u);
}

void countdown_manager_cancel_all(void)
{
    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        for (uint8_t i = 0u; i < COUNTDOWN_LIGHT_COUNT; i++)
        {
            s_light_timers[i] = 0u;
        }
        xSemaphoreGive(g_config_mutex);
    }
}

void countdown_manager_tick(uint32_t elapsed_ms)
{
    s_ms_accumulator += elapsed_ms;
    if (s_ms_accumulator < 1000u)
    {
        return;
    }

    uint32_t elapsed_sec = s_ms_accumulator / 1000u;
    s_ms_accumulator %= 1000u;

    if (xSemaphoreTake(g_config_mutex, pdMS_TO_TICKS(50)) != pdTRUE)
    {
        return;
    }

    /* Process Light timers 1..6 */
    for (uint8_t i = 0u; i < COUNTDOWN_LIGHT_COUNT; i++)
    {
        if (s_light_timers[i] > 0u)
        {
            if (s_light_timers[i] <= elapsed_sec)
            {
                s_light_timers[i] = 0u;
                uint8_t sw_id = (uint8_t) (i + 1u);
                bool current_state = light_manager_get_switch(sw_id);
                bool new_state = !current_state;
                LOG_INFO("Countdown EXPIRED: Light SW%u toggled to %s", sw_id, new_state ? "ON" : "OFF");

                light_manager_set_switch(sw_id, new_state);
                backlight_manager_notify_switch_state(sw_id, new_state);

                tuya_dp_notify_switch_changed(sw_id, new_state);
                tuya_dp_notify_countdown_changed(sw_id, 0u);
            }
            else
            {
                s_light_timers[i] -= elapsed_sec;
            }
        }
    }

    xSemaphoreGive(g_config_mutex);
}
