/*
 * ir_driver.c
 *
 * *** FRONT END REPLACED: hardware GPT capture instead of software
 * polling *** -- decode_rc5()/decode_nec() below are UNCHANGED from
 * the previously-validated version (16/16 buttons decoding correctly)
 * -- only how pulses get INTO s_pulses[] changed. See ir_driver.h.
 */

#include "drivers/ir_driver.h"
#include "hal/hal_ir_capture.h"
#include "hal_data.h"
#include "app/app_log.h"

#include <string.h>

#define IR_MAX_PULSES        (70u)
#define IR_IDLE_TIMEOUT_MS   (5u)   /* no new edge within this window = frame complete.
                                       Comfortably shorter than RC5's own ~4.5ms end-of-frame
                                       idle convention, and MUCH shorter than the old software
                                       polling's exposure window to touch_thread preemption. */

typedef struct
{
    uint16_t duration_us;
    uint8_t  level;
} ir_pulse_t;

static ir_pulse_t s_pulses[IR_MAX_PULSES];
static uint32_t   s_pulse_count = 0u;

void ir_driver_init(void)
{
    hal_ir_capture_init();

    /* P103 pin config (input, pull-up) is owned by the GPT5/GTIOC5A
     * peripheral mapping now, configured in the Pins tab alongside
     * the GPT stack -- no separate R_IOPORT_PinCfg needed here. */

    ir_driver_reset_decoder();

    LOG_INFO("IR Driver initialized on P103 (LF0038KAHA, GPT5 hardware capture)");
}

void ir_driver_reset_decoder(void)
{
    s_pulse_count = 0u;
}

/* --------------------------------------------------------------------------
 * RC5 Manchester Decoder (14 bits = 28 half-bits) -- UNCHANGED
 * -------------------------------------------------------------------------- */
static bool decode_rc5(const ir_pulse_t *pulses, uint32_t count, ir_message_t *out_msg)
{
    if ((pulses == NULL) || (count < 6u))
    {
        return false;
    }

    bool half_bits[32];
    uint32_t hb_count = 0u;

    half_bits[hb_count++] = true;

    for (uint32_t i = 0u; i < count && hb_count < 28u; i++)
    {
        uint32_t dur = pulses[i].duration_us;
        uint32_t k = 0u;

        if ((i == 0u) && (dur < 350u))
        {
            if ((count > 1u) && (pulses[1].duration_us >= 1300u))
            {
                k = 2u;
            }
            else
            {
                k = 1u;
            }
        }
        else if ((dur >= 250u) && (dur < 1300u))
        {
            k = 1u;
        }
        else if ((dur >= 1300u) && (dur <= 2600u))
        {
            k = 2u;
        }
        else if ((dur > 2600u) && (pulses[i].level == true))
        {
            break;
        }
        else
        {
            return false;
        }

        while ((k > 0u) && (hb_count < 28u))
        {
            half_bits[hb_count++] = pulses[i].level;
            k--;
        }
    }

    while (hb_count < 28u)
    {
        half_bits[hb_count++] = true;
    }

    uint8_t bits[14];
    for (uint32_t i = 0u; i < 14u; i++)
    {
        bool h0 = half_bits[i * 2u];
        bool h1 = half_bits[(i * 2u) + 1u];

        if (h0 && !h1)
        {
            bits[i] = 1u;
        }
        else if (!h0 && h1)
        {
            bits[i] = 0u;
        }
        else
        {
            return false;
        }
    }

    if (bits[0] != 1u)
    {
        return false;
    }

    bool toggle = (bits[2] != 0u);

    uint8_t address = 0u;
    for (uint8_t i = 0u; i < 5u; i++)
    {
        address = (uint8_t)((address << 1u) | bits[3u + i]);
    }

    uint8_t command = 0u;
    for (uint8_t i = 0u; i < 6u; i++)
    {
        command = (uint8_t)((command << 1u) | bits[8u + i]);
    }

    if (bits[1] == 0u)
    {
        command = (uint8_t)(command | (1u << 6u));
    }

    out_msg->protocol = IR_PROTO_RC5;
    out_msg->toggle   = toggle;
    out_msg->address  = address;
    out_msg->command  = command;
    out_msg->raw_code = (uint16_t)(((uint16_t)(toggle ? 1u : 0u) << 11u) |
                                  ((uint16_t)address << 6u) |
                                  ((uint16_t)(command & 0x3Fu)));
    out_msg->repeat   = false;

    return true;
}

/* --------------------------------------------------------------------------
 * NEC Protocol Decoder (32 bits) -- UNCHANGED
 * -------------------------------------------------------------------------- */
static bool decode_nec(const ir_pulse_t *pulses, uint32_t count, ir_message_t *out_msg)
{
    if ((pulses == NULL) || (count < 66u))
    {
        return false;
    }

    if ((pulses[0].duration_us < 7000u) || (pulses[0].duration_us > 11000u) || pulses[0].level != false)
    {
        return false;
    }

    if ((pulses[1].duration_us < 3000u) || (pulses[1].duration_us > 6000u) || pulses[1].level != true)
    {
        return false;
    }

    uint32_t raw_32 = 0u;
    uint32_t pulse_idx = 2u;

    for (uint32_t bit = 0u; bit < 32u; bit++)
    {
        if (pulse_idx + 1u >= count)
        {
            return false;
        }

        uint32_t low_dur  = pulses[pulse_idx].duration_us;
        uint32_t high_dur = pulses[pulse_idx + 1u].duration_us;
        pulse_idx += 2u;

        if ((low_dur < 250u) || (low_dur > 900u))
        {
            return false;
        }

        if ((high_dur >= 250u) && (high_dur <= 900u))
        {
            /* Bit 0 */
        }
        else if ((high_dur >= 1100u) && (high_dur <= 2200u))
        {
            raw_32 |= (1u << bit);
        }
        else
        {
            return false;
        }
    }

    uint8_t addr     = (uint8_t)(raw_32 & 0xFFu);
    uint8_t addr_inv = (uint8_t)((raw_32 >> 8u) & 0xFFu);
    uint8_t cmd      = (uint8_t)((raw_32 >> 16u) & 0xFFu);
    uint8_t cmd_inv  = (uint8_t)((raw_32 >> 24u) & 0xFFu);

    if (cmd != (uint8_t)(~cmd_inv))
    {
        return false;
    }

    (void)addr_inv;

    out_msg->protocol = IR_PROTO_NEC;
    out_msg->toggle   = false;
    out_msg->address  = addr;
    out_msg->command  = cmd;
    out_msg->raw_code = (uint16_t)(((uint16_t)addr << 8u) | (uint16_t)cmd);
    out_msg->repeat   = false;

    return true;
}

/* --------------------------------------------------------------------------
 * Capture-driven frame assembly (REPLACES the old polling loop)
 * -------------------------------------------------------------------------- */
bool ir_driver_process(ir_message_t *out_msg)
{
    if (out_msg == NULL)
    {
        return false;
    }

    /* Drain everything currently buffered, non-blocking. */
    ir_edge_t edge;
    bool got_edge = false;

    while (hal_ir_capture_pop_edge(&edge))
    {
        /* Ignore the pre-frame idle HIGH period so s_pulses[0] is the first active LOW pulse (P0) */
        if ((s_pulse_count == 0u) && (edge.level == true))
        {
            continue;
        }

        got_edge = true;

        if (s_pulse_count < IR_MAX_PULSES)
        {
            s_pulses[s_pulse_count].duration_us = (uint16_t) edge.duration_us;
            s_pulses[s_pulse_count].level       = (uint8_t) (edge.level ? 1u : 0u);
            s_pulse_count++;
        }
        /* else: silently drop -- pathological frame, decode will
         * likely fail anyway; better than overrunning the buffer. */
    }

    if (got_edge)
    {
        return false; /* still actively receiving -- not complete yet */
    }

    /* Nothing pending right now. Block briefly for the next edge; if
     * none arrives within IR_IDLE_TIMEOUT_MS, treat that silence as
     * frame-complete (mirrors the old software timeout concept, but
     * driven by the semaphore instead of polling the pin).
     */
    if (xSemaphoreTake(g_ir_capture_semaphore, pdMS_TO_TICKS(IR_IDLE_TIMEOUT_MS)) == pdTRUE)
    {
        return false; /* an edge arrived right as we were about to give up --
                        * let the next call's drain loop pick it up */
    }

    if (s_pulse_count == 0u)
    {
        return false; /* genuinely idle, nothing to decode */
    }

    if (s_pulse_count >= 4u)
    {
        LOG_INFO("IR Burst on P103: %lu pulses (P0=%luus %s, P1=%luus %s, P2=%luus %s)",
                 (unsigned long) s_pulse_count,
                 (unsigned long) s_pulses[0].duration_us, s_pulses[0].level ? "HIGH" : "LOW",
                 (unsigned long) s_pulses[1].duration_us, s_pulses[1].level ? "HIGH" : "LOW",
                 (unsigned long) s_pulses[2].duration_us, s_pulses[2].level ? "HIGH" : "LOW");
    }

    bool decoded = decode_rc5(s_pulses, s_pulse_count, out_msg);
    if (!decoded)
    {
        decoded = decode_nec(s_pulses, s_pulse_count, out_msg);
    }

    s_pulse_count = 0u; /* always reset -- whether decode succeeded or the
                          * frame was garbage, we're done with these pulses */

    return decoded;
}

uint8_t ir_driver_get_address(uint16_t rc5_code)
{
    return (uint8_t)((rc5_code >> 6u) & 0x1Fu);
}

uint8_t ir_driver_get_command(uint16_t rc5_code)
{
    return (uint8_t)(rc5_code & 0x3Fu);
}

bool ir_driver_get_toggle(uint16_t rc5_code)
{
    return ((rc5_code >> 11u) & 0x01u) != 0u;
}

uint16_t ir_driver_strip_toggle(uint16_t rc5_code)
{
    return (uint16_t)(rc5_code & 0x07FFu);
}

bool ir_driver_is_scene_button(uint8_t address, uint8_t command, uint8_t *out_scene_index)
{
    if ((address != IR_FIXED_ADDR) && (address != IR_REMOTE_R2_ADDR))
    {
        return false;
    }

    uint8_t idx = 0u;
    switch (command)
    {
        case IR_CMD_SCENE1: idx = 0u; break;
        case IR_CMD_SCENE2: idx = 1u; break;
        case IR_CMD_SCENE3: idx = 2u; break;
        case IR_CMD_SCENE4: idx = 3u; break;
        case IR_CMD_SCENE5: idx = 4u; break;
        case IR_CMD_SCENE6: idx = 5u; break;
        case IR_CMD_SCENE7: idx = 6u; break;
        case IR_CMD_SCENE8: idx = 7u; break;
        default: return false;
    }

    if (out_scene_index != NULL)
    {
        *out_scene_index = idx;
    }
    return true;
}

bool ir_driver_is_fixed_control_button(uint8_t address, uint8_t command)
{
    if (address == IR_FIXED_ADDR)
    {
        switch (command)
        {
            case IR_CMD_ALL_ON:
            case IR_CMD_ALL_OFF:
                return true;
            default:
                return false;
        }
    }
    else if (address == IR_REMOTE_R2_ADDR)
    {
        switch (command)
        {
            case IR_CMD_BACKLIGHT_TOGGLE:
            case IR_CMD_BACKLIGHT_ON_COLOR:
            case IR_CMD_BACKLIGHT_OFF_COLOR:
            case IR_CMD_BACKLIGHT_BRIGHTNESS_INC:
            case IR_CMD_BACKLIGHT_BRIGHTNESS_DEC:
            case IR_CMD_CHILD_LOCK_TOGGLE:
                return true;
            default:
                return false;
        }
    }

    return false;
}