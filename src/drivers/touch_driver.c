/*
 * touch_driver.c
 *
 * See touch_driver.h. Slider handling removed entirely -- this board
 * has no slider hardware. Long-press timing logic is unchanged from
 * the original 11-element version, just applied to 6 elements instead
 * of 7, and no F1 (switch_id 7) exists on this board.
 */

#include "drivers/touch_driver.h"
#include "hal/hal_touch.h"
#include "app/app_common.h"

#include <string.h>

#define LONG_PRESS_5S_MS    (5000u)
#define LONG_PRESS_10S_MS   (10000u)

typedef struct
{
    bool     was_touched;
    uint32_t press_start_tick_ms;
    bool     long_5s_fired;
    bool     long_10s_fired;
} button_state_t;

static button_state_t s_button[6]; /* S1-S6 -- indices 0-5 */

static uint32_t now_ms(void)
{
    return (uint32_t) (xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static void post_event(touch_event_type_t type, uint8_t switch_id)
{
    touch_event_t evt = { .type = type, .switch_id = switch_id };
    xQueueSend(g_touch_event_queue, &evt, 0);
}

static void process_button(uint8_t index_0to5, uint64_t status, uint8_t switch_id_1to6)
{
    button_state_t *st      = &s_button[index_0to5];
    bool             touched = ((status >> index_0to5) & 0x1ULL) != 0u;

    if (touched && !st->was_touched)
    {
        st->press_start_tick_ms = now_ms();
        st->long_5s_fired       = false;
        st->long_10s_fired      = false;
    }
    else if (!touched && st->was_touched)
    {
        if (!st->long_5s_fired && !st->long_10s_fired)
        {
            post_event((touch_event_type_t) (TOUCH_EVT_S1_SHORT_PRESS + (switch_id_1to6 - 1u)),
                       switch_id_1to6);
        }
    }
    else if (touched)
    {
        uint32_t held_ms = now_ms() - st->press_start_tick_ms;

        if (!st->long_5s_fired && (held_ms >= LONG_PRESS_5S_MS))
        {
            st->long_5s_fired = true;
            post_event(TOUCH_EVT_LONG_PRESS_5S, switch_id_1to6);
        }

        if (!st->long_10s_fired && (held_ms >= LONG_PRESS_10S_MS))
        {
            st->long_10s_fired = true;
            post_event(TOUCH_EVT_LONG_PRESS_10S, switch_id_1to6);
        }
    }

    st->was_touched = touched;
}

void touch_driver_init(void)
{
    memset(s_button, 0, sizeof(s_button));
    hal_touch_init();
}

void touch_driver_scan(void)
{
    if (!hal_touch_scan_and_wait(100u))
    {
        return;
    }

    uint64_t status = hal_touch_get_button_status();

    for (uint8_t i = 0u; i < 6u; i++)
    {
        process_button(i, status, (uint8_t) (i + 1u));
    }
}
