/*
 * app_queue_init.c
 *
 * Hand-creates the 4 project queues using STATIC allocation (matching
 * the RAM-budget benefit the FSP wizard would otherwise give us), but
 * with length and item-size driven by macros / sizeof() instead of
 * literal numbers typed into a GUI field. See the ownership note at
 * the top of app_common.h for why queues specifically are hand-rolled
 * while event groups/mutexes/semaphores use the Configurator wizard.
 *
 * Requires configSUPPORT_STATIC_ALLOCATION == 1 in
 * ra_cfg/aws/FreeRTOSConfig.h (already enabled if you've created any
 * static wizard objects, e.g. the event groups/mutexes/semaphores).
 */

#include "app/app_common.h"

/* --------------------------------------------------------------------- */
/* Static backing memory -- one control block + one storage array per     */
/* queue. Storage size is QUEUE_LEN * sizeof(item), computed by the       */
/* compiler, not typed by hand.                                          */
/* --------------------------------------------------------------------- */
static StaticQueue_t g_touch_event_queue_cb;
static uint8_t       g_touch_event_queue_storage[QUEUE_LEN_TOUCH_EVENTS * sizeof(touch_event_t)];

static StaticQueue_t g_ir_event_queue_cb;
static uint8_t       g_ir_event_queue_storage[QUEUE_LEN_IR_EVENTS * sizeof(ir_event_t)];

static StaticQueue_t g_tuya_dp_queue_cb;
static uint8_t       g_tuya_dp_queue_storage[QUEUE_LEN_TUYA_DP_CMDS * sizeof(tuya_dp_cmd_t)];

static StaticQueue_t g_rgb_update_queue_cb;
static uint8_t       g_rgb_update_queue_storage[QUEUE_LEN_RGB_UPDATES * sizeof(rgb_update_t)];

/* --------------------------------------------------------------------- */
/* Global handles (declared extern in app_common.h)                       */
/* --------------------------------------------------------------------- */
QueueHandle_t g_touch_event_queue = NULL;
QueueHandle_t g_ir_event_queue    = NULL;
QueueHandle_t g_tuya_dp_queue     = NULL;
QueueHandle_t g_rgb_update_queue  = NULL;

bool app_queue_init(void)
{
    g_touch_event_queue = xQueueCreateStatic(QUEUE_LEN_TOUCH_EVENTS,
                                              sizeof(touch_event_t),
                                              g_touch_event_queue_storage,
                                              &g_touch_event_queue_cb);

    g_ir_event_queue = xQueueCreateStatic(QUEUE_LEN_IR_EVENTS,
                                           sizeof(ir_event_t),
                                           g_ir_event_queue_storage,
                                           &g_ir_event_queue_cb);

    g_tuya_dp_queue = xQueueCreateStatic(QUEUE_LEN_TUYA_DP_CMDS,
                                          sizeof(tuya_dp_cmd_t),
                                          g_tuya_dp_queue_storage,
                                          &g_tuya_dp_queue_cb);

    g_rgb_update_queue = xQueueCreateStatic(QUEUE_LEN_RGB_UPDATES,
                                             sizeof(rgb_update_t),
                                             g_rgb_update_queue_storage,
                                             &g_rgb_update_queue_cb);

    /* xQueueCreateStatic() only returns NULL if passed a NULL buffer
     * pointer -- since our buffers are compile-time arrays, not runtime
     * allocations, a NULL result here means a coding mistake (e.g. a
     * typo'd argument), not an out-of-memory condition. Still checked,
     * since proceeding with a NULL queue handle would fault the first
     * time any thread calls xQueueSend()/xQueueReceive() on it.
     */
    return (g_touch_event_queue != NULL) &&
           (g_ir_event_queue    != NULL) &&
           (g_tuya_dp_queue     != NULL) &&
           (g_rgb_update_queue  != NULL);
}
