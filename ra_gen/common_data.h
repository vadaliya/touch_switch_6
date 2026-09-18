/* generated common header file - do not edit */
#ifndef COMMON_DATA_H_
#define COMMON_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "r_sci_uart.h"
#include "r_uart_api.h"
#include "r_dtc.h"
#include "r_transfer_api.h"
#include "r_ctsu.h"
#include "r_ctsu_api.h"
#include "rm_touch.h"
#include "rm_touch_api.h"
#include "r_icu.h"
#include "r_external_irq_api.h"
#include "r_ioport.h"
#include "bsp_pin_cfg.h"
FSP_HEADER
/** UART on SCI Instance. */
extern const uart_instance_t g_uart_qe;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_uart_instance_ctrl_t g_uart_qe_ctrl;
extern const uart_cfg_t g_uart_qe_cfg;
extern const sci_uart_extended_cfg_t g_uart_qe_cfg_extend;

#ifndef touch_uart_callback
void touch_uart_callback(uart_callback_args_t *p_args);
#endif
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer1;

/** Access the DTC instance using these structures when calling API functions directly (::p_api is not used). */
extern dtc_instance_ctrl_t g_transfer1_ctrl;
extern const transfer_cfg_t g_transfer1_cfg;
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer0;

/** Access the DTC instance using these structures when calling API functions directly (::p_api is not used). */
extern dtc_instance_ctrl_t g_transfer0_ctrl;
extern const transfer_cfg_t g_transfer0_cfg;
/** External IRQ on ICU Instance. */
extern const external_irq_instance_t g_external_irq0;

/** Access the ICU instance using these structures when calling API functions directly (::p_api is not used). */
extern icu_instance_ctrl_t g_external_irq0_ctrl;
extern const external_irq_cfg_t g_external_irq0_cfg;

#ifndef NULL
void NULL(external_irq_callback_args_t *p_args);
#endif
#define IOPORT_CFG_NAME g_bsp_pin_cfg
#define IOPORT_CFG_OPEN R_IOPORT_Open
#define IOPORT_CFG_CTRL g_ioport_ctrl

/* IOPORT Instance */
extern const ioport_instance_t g_ioport;

/* IOPORT control structure. */
extern ioport_instance_ctrl_t g_ioport_ctrl;
extern EventGroupHandle_t g_heartbeat_event_group;
extern EventGroupHandle_t g_system_state_event_group;
extern SemaphoreHandle_t g_eeprom_mutex;
extern SemaphoreHandle_t g_config_mutex;
extern SemaphoreHandle_t g_ir_capture_semaphore;
extern SemaphoreHandle_t g_uart_rx_semaphore;
extern SemaphoreHandle_t g_ctsu_scan_complete_semaphore;
extern SemaphoreHandle_t g_touch_scan_complete_semaphore;
extern SemaphoreHandle_t g_flash_op_complete_semaphore;
extern SemaphoreHandle_t g_uart_tx_complete_semaphore;
void g_common_init(void);
FSP_FOOTER
#endif /* COMMON_DATA_H_ */
