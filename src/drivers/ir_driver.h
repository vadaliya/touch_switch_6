#ifndef IR_DRIVER_H
#define IR_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#define IR_RC5_BIT_COUNT         (14u)
#define IR_RC5_HALF_BIT_US       (889u)
#define IR_RC5_FULL_BIT_US       (1778u)

typedef enum
{
    IR_PROTO_NONE = 0,
    IR_PROTO_RC5,
    IR_PROTO_NEC
} ir_protocol_t;

typedef struct
{
    ir_protocol_t protocol;
    uint16_t      raw_code;
    uint8_t       address;
    uint8_t       command;
    bool          toggle;
    bool          repeat;
} ir_message_t;

/**
 * ir_driver_init
 * Initializes GPT5 hardware edge capture (via hal_ir_capture_init())
 * and the P103 pin. Hardware timestamping replaces the old software
 * polling loop -- see project history for why (touch_thread's CTSU
 * scans were preempting the polling loop mid-pulse on fast taps).
 */
void ir_driver_init(void);

/**
 * ir_driver_reset_decoder
 * Resets internal state machine and repeat debounce timers.
 */
void ir_driver_reset_decoder(void);

/**
 * ir_driver_process
 *
 * Drains any newly captured edges (non-blocking) into the pulse
 * buffer. If none are pending, blocks briefly (IR_IDLE_TIMEOUT_MS) --
 * a timeout with no new edge is what signals "frame complete" now,
 * replacing the old explicit idle-HIGH-duration check. Call this in a
 * tight loop from ir_rx_thread; it self-paces via the internal wait,
 * no external vTaskDelay/taskYIELD needed.
 *
 * @param out_msg  populated on successful decode
 * @return true if a valid new IR command was decoded this call.
 */
bool ir_driver_process(ir_message_t *out_msg);

uint8_t ir_driver_get_address(uint16_t rc5_code);
uint8_t ir_driver_get_command(uint16_t rc5_code);
bool    ir_driver_get_toggle(uint16_t rc5_code);
uint16_t ir_driver_strip_toggle(uint16_t rc5_code);

/* Fixed IR Remote Commands (RC5 Address 13) */
#define IR_FIXED_ADDR                   (13u)

#define IR_CMD_ALL_ON                   (13u)   /* ALL ON: 0x034D / 0x0B4D */
#define IR_CMD_ALL_OFF                  (12u)   /* ALL OFF: 0x0B4C / 0x034C */

/* Toggle-stripped 11-bit raw codes: (Address << 6) | Command */
#define IR_CODE_FIXED_ALL_ON            (0x034Du)
#define IR_CODE_FIXED_ALL_OFF           (0x034Cu)

/* Fixed IR Remote R2 Commands (RC5 Address 15) */
#define IR_REMOTE_R2_ADDR                      (15u)

#define IR_CMD_BACKLIGHT_TOGGLE                (47u)   /* L11: 0x03EF / 0x0BEF */
#define IR_CMD_BACKLIGHT_ON_COLOR              (27u)   /* L12: 0x03DB / 0x0BDB */
#define IR_CMD_BACKLIGHT_OFF_COLOR             (46u)   /* L13: 0x03EE / 0x0BEE */
#define IR_CMD_BACKLIGHT_BRIGHTNESS_INC        (2u)    /* L14: 0x03C2 / 0x0BC2 */
#define IR_CMD_BACKLIGHT_BRIGHTNESS_DEC        (49u)   /* L15: 0x03F1 / 0x0BF1 */
#define IR_CMD_CHILD_LOCK_TOGGLE               (48u)   /* L16: 0x03F0 / 0x0BF0 */

#define IR_CODE_FIXED_BACKLIGHT_TOGGLE         (0x03EFu)
#define IR_CODE_FIXED_BACKLIGHT_ON_COLOR       (0x03DBu)
#define IR_CODE_FIXED_BACKLIGHT_OFF_COLOR      (0x03EEu)
#define IR_CODE_FIXED_BACKLIGHT_BRIGHTNESS_INC (0x03C2u)
#define IR_CODE_FIXED_BACKLIGHT_BRIGHTNESS_DEC (0x03F1u)
#define IR_CODE_FIXED_CHILD_LOCK_TOGGLE        (0x03F0u)

/* Dedicated Scene Commands (Shared by Remote R1 Addr 13 and Remote R2 Addr 15) */
#define IR_CMD_SCENE1                          (26u)   /* R1: 0x035A, R2: 0x03DA */
#define IR_CMD_SCENE2                          (25u)   /* R1: 0x0359, R2: 0x03D9 */
#define IR_CMD_SCENE3                          (24u)   /* R1: 0x0358, R2: 0x03D8 */
#define IR_CMD_SCENE4                          (23u)   /* R1: 0x0357, R2: 0x03D7 */
#define IR_CMD_SCENE5                          (22u)   /* R1: 0x0356, R2: 0x03D6 */
#define IR_CMD_SCENE6                          (20u)   /* R1: 0x0354, R2: 0x03D4 */
#define IR_CMD_SCENE7                          (19u)   /* R1: 0x0353, R2: 0x03D3 */
#define IR_CMD_SCENE8                          (18u)   /* R1: 0x0352, R2: 0x03D2 */

/* Remote R1 Dedicated Scene Codes (Addr 13) */
#define IR_CODE_R1_SCENE1                      (0x035Au)
#define IR_CODE_R1_SCENE2                      (0x0359u)
#define IR_CODE_R1_SCENE3                      (0x0358u)
#define IR_CODE_R1_SCENE4                      (0x0357u)
#define IR_CODE_R1_SCENE5                      (0x0356u)
#define IR_CODE_R1_SCENE6                      (0x0354u)
#define IR_CODE_R1_SCENE7                      (0x0353u)
#define IR_CODE_R1_SCENE8                      (0x0352u)

/* Remote R2 Dedicated Scene Codes (Addr 15) */
#define IR_CODE_R2_SCENE1                      (0x03DAu)
#define IR_CODE_R2_SCENE2                      (0x03D9u)
#define IR_CODE_R2_SCENE3                      (0x03D8u)
#define IR_CODE_R2_SCENE4                      (0x03D7u)
#define IR_CODE_R2_SCENE5                      (0x03D6u)
#define IR_CODE_R2_SCENE6                      (0x03D4u)
#define IR_CODE_R2_SCENE7                      (0x03D3u)
#define IR_CODE_R2_SCENE8                      (0x03D2u)

/* Default Switch IR Codes (Remote L1-L6 on Addr 13) */
#define IR_DEFAULT_CODE_SWITCH_1               (0x0362u)   /* Remote L1: Addr=13, Cmd=34 (0x0362 / 0x0B62) */
#define IR_DEFAULT_CODE_SWITCH_2               (0x0351u)   /* Remote L2: Addr=13, Cmd=17 (0x0351 / 0x0B51) */
#define IR_DEFAULT_CODE_SWITCH_3               (0x036Bu)   /* Remote L3: Addr=13, Cmd=43 (0x036B / 0x0B6B) */
#define IR_DEFAULT_CODE_SWITCH_4               (0x0343u)   /* Remote L4: Addr=13, Cmd=3  (0x0343 / 0x0B43) */
#define IR_DEFAULT_CODE_SWITCH_5               (0x036Cu)   /* Remote L5: Addr=13, Cmd=44 (0x036C / 0x0B6C) */
#define IR_DEFAULT_CODE_SWITCH_6               (0x0346u)   /* Remote L6: Addr=13, Cmd=6  (0x0346 / 0x0B46) */

bool ir_driver_is_scene_button(uint8_t address, uint8_t command, uint8_t *out_scene_index);
bool ir_driver_is_fixed_control_button(uint8_t address, uint8_t command);

#endif /* IR_DRIVER_H */