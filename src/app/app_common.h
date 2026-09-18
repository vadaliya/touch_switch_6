/*
 * app_common.h
 *
 * Shared definitions for FSP-generated FreeRTOS threads.
 *
 * IPC OBJECT OWNERSHIP -- deliberately split between two mechanisms:
 *
 *   - QUEUES (4 of them) are hand-created in app_queue_init.c using
 *     xQueueCreateStatic() with macro-driven lengths and sizeof()-driven
 *     item sizes. This is intentional: the FSP Configurator's Queue
 *     object only accepts a literal numeric byte count for item size,
 *     with no way to reference a macro or sizeof() expression, and that
 *     generated number never re-syncs itself if the struct changes.
 *     Hand-written xQueueCreateStatic() has neither limitation.
 *
 *   - EVENT GROUPS, MUTEXES, and BINARY SEMAPHORES have no size-related
 *     properties in the wizard (just Symbol + Allocation), so there's
 *     no macro/sizeof() to lose by using the wizard for these. They are
 *     created via the FSP Configurator (New Object, Static allocation)
 *     and declared for us in the FSP-generated "common_data.h" --
 *     included below instead of re-declared here.
 *
 * This header contains no hardware/HAL includes so it can be shared
 * freely across driver, middleware, and application layers.
 */

#ifndef APP_COMMON_H
#define APP_COMMON_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "timers.h"
#include "app/app_log.h"

#include "common_data.h"   /* FSP-generated: declares g_heartbeat_event_group,
                             * g_system_state_event_group, g_eeprom_mutex,
                             * g_config_mutex, g_ir_capture_semaphore,
                             * g_uart_rx_semaphore, g_uart_tx_complete_semaphore,
                             * g_flash_op_complete_semaphore -- all created via
                             * the Configurator's New Object, Static allocation.
                             * NOTE: g_touch_scan_complete_semaphore is NOT
                             * needed -- rm_touch's QE-managed config has no
                             * exposed Callback field, so hal_touch.c polls
                             * QE's own g_qe_touch_flag directly instead (see
                             * hal_touch.c) rather than using a semaphore.
                             * Their StaticEventGroup_t / StaticSemaphore_t
                             * backing memory and g_common_init() live in
                             * common_data.c, fully auto-generated -- never
                             * hand-edited, never called manually (FSP's
                             * generated startup sequence calls it before
                             * any thread runs).
                             */

#include <stdint.h>
#include <stdbool.h>

/* --------------------------------------------------------------------- */
/* QE Touch Tuning Mode Macro:                                           */
/* 1 = QE Touch serial tuning mode (UART callback -> QE Touch tuning)    */
/* 0 = Normal operation (UART callback -> Tuya Wi-Fi module)             */
/* --------------------------------------------------------------------- */
#ifndef QE_TUNNING_TESTING
#define QE_TUNNING_TESTING       (0)
#endif

/* --------------------------------------------------------------------- */
/* Queue lengths -- used directly by app_queue_init.c, and referenced     */
/* here as the single source of truth if any other file needs to know    */
/* queue depth (e.g. for a "queue nearly full" diagnostic).               */
/* --------------------------------------------------------------------- */
#define QUEUE_LEN_TOUCH_EVENTS   (8u)
#define QUEUE_LEN_IR_EVENTS      (4u)
#define QUEUE_LEN_TUYA_DP_CMDS   (8u)
#define QUEUE_LEN_RGB_UPDATES    (16u)

/* --------------------------------------------------------------------- */
/* Touch event types (touch_driver -> app layer)                         */
/* --------------------------------------------------------------------- */
typedef enum
{
    TOUCH_EVT_NONE = 0,
    TOUCH_EVT_S1_SHORT_PRESS,
    TOUCH_EVT_S2_SHORT_PRESS,
    TOUCH_EVT_S3_SHORT_PRESS,
    TOUCH_EVT_S4_SHORT_PRESS,
    TOUCH_EVT_S5_SHORT_PRESS,
    TOUCH_EVT_S6_SHORT_PRESS,
    TOUCH_EVT_LONG_PRESS_5S,      /* IR learn trigger; source switch in .switch_id */
    TOUCH_EVT_LONG_PRESS_10S,     /* Wi-Fi pairing trigger */
    TOUCH_EVT_LONG_PRESS_15S,     /* Master Switch mode toggle trigger (Switch 1) */
} touch_event_type_t;

typedef struct
{
    touch_event_type_t type;
    uint8_t            switch_id;   /* 1-6 = S1-S6 */
} touch_event_t;
/* Item size for this queue is sizeof(touch_event_t) -- computed by the
 * compiler in app_queue_init.c, not typed as a literal anywhere. If you
 * add a field to this struct, the queue automatically resizes its item
 * slots on the next build; nothing else to update. */

/* --------------------------------------------------------------------- */
/* IR event types (ir_driver -> app layer)                                */
/* --------------------------------------------------------------------- */
typedef struct
{
    uint16_t raw_code;      /* decoded RC5 command */
    uint32_t timestamp_ms;
} ir_event_t;

/* --------------------------------------------------------------------- */
/* Tuya DP command (tuya_uart middleware -> app layer)                    */
/* --------------------------------------------------------------------- */
typedef struct
{
    uint8_t  dpid;
    uint8_t  length;
    uint8_t  value[8];   /* sized for the largest DP payload used in this spec */
} tuya_dp_cmd_t;

/* --------------------------------------------------------------------- */
/* RGB update request (any manager -> rgb_led_driver via RGB thread)      */
/* --------------------------------------------------------------------- */
typedef enum
{
    RGB_UPDATE_SWITCH_STATE,   /* one LED, solid color per ON/OFF color config */
    RGB_UPDATE_WIFI_STATUS,    /* Wi-Fi status LED, blink pattern */
    RGB_UPDATE_GLOBAL_COLOR,   /* ON or OFF color changed, full refresh */
    RGB_UPDATE_BRIGHTNESS,     /* global brightness changed, full refresh */
    RGB_UPDATE_BACKLIGHT_ONOFF,/* whole backlight system enable/disable */
    RGB_UPDATE_IR_LEARN_BLINK  /* one or more LEDs blinking during learn mode */
} rgb_update_type_t;

typedef struct
{
    rgb_update_type_t type;
    uint16_t          led_mask;  /* bitmask of affected LEDs on backlight chain */
    uint8_t           param;     /* switch_id or on/off flag */
} rgb_update_t;

/* --------------------------------------------------------------------- */
/* System heartbeat bits -- each critical thread sets its bit once per    */
/* healthy loop iteration; System thread clears them after checking and  */
/* only then refreshes the watchdog.                                     */
/* --------------------------------------------------------------------- */
#define HEARTBEAT_BIT_TOUCH      (1u << 0)
#define HEARTBEAT_BIT_IR_RX      (1u << 1)
#define HEARTBEAT_BIT_RGB        (1u << 2)
#define HEARTBEAT_BIT_TUYA_UART  (1u << 3)

#define HEARTBEAT_ALL_BITS \
    (HEARTBEAT_BIT_TOUCH | HEARTBEAT_BIT_IR_RX | HEARTBEAT_BIT_RGB | HEARTBEAT_BIT_TUYA_UART)

/* System state event group bits */
#define SYS_BIT_WIFI_CONNECTED       (1u << 0)
#define SYS_BIT_WIFI_SMART_CONFIG    (1u << 1)
#define SYS_BIT_WIFI_AP_MODE         (1u << 2)
#define SYS_BIT_BACKLIGHT_ENABLED    (1u << 3)
#define SYS_BIT_CHILD_LOCK           (1u << 4)

/* --------------------------------------------------------------------- */
/* Queues -- hand-created; see app_queue_init.c. Declared here (not in    */
/* common_data.h, since these are NOT wizard-generated objects).          */
/* --------------------------------------------------------------------- */
extern QueueHandle_t      g_touch_event_queue;   /* touch_thread        -> app managers        */
extern QueueHandle_t      g_ir_event_queue;      /* ir_rx_thread        -> ir_learning_manager  */
extern QueueHandle_t      g_tuya_dp_queue;       /* tuya_uart_thread    -> DP handler dispatch  */
extern QueueHandle_t      g_rgb_update_queue;    /* any manager         -> rgb_thread           */

/* --------------------------------------------------------------------- */
/* One-time queue creation -- called from hal_entry.c before any thread   */
/* body runs. (Event groups/mutexes/semaphores need no equivalent call:  */
/* they're wizard-generated and already live by the time hal_entry()     */
/* executes -- see the common_data.h comment above.)                     */
/* Returns false if any queue failed to allocate its static buffer's      */
/* backing memory would be a compile-time array, so failure here really   */
/* only indicates a bug (e.g. NULL storage pointer), not an OOM condition.*/
/* --------------------------------------------------------------------- */
bool app_queue_init(void);

#endif /* APP_COMMON_H */
