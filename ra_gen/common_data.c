/* generated common source file - do not edit */
#include "common_data.h"
sci_uart_instance_ctrl_t g_uart_qe_ctrl;

baud_setting_t g_uart_qe_baud_setting =
        {
        /* Baud rate calculated with 0.160% error. */.semr_baudrate_bits_b.abcse = 0,
          .semr_baudrate_bits_b.abcs = 0, .semr_baudrate_bits_b.bgdm = 1, .cks = 0, .brr = 12, .mddr = (uint8_t) 256, .semr_baudrate_bits_b.brme =
                  false };

/** UART extended configuration for UARTonSCI HAL driver */
const sci_uart_extended_cfg_t g_uart_qe_cfg_extend =
{ .clock = SCI_UART_CLOCK_INT, .rx_edge_start = SCI_UART_START_BIT_FALLING_EDGE, .noise_cancel =
          SCI_UART_NOISE_CANCELLATION_DISABLE,
  .rx_fifo_trigger = SCI_UART_RX_FIFO_TRIGGER_MAX, .p_baud_setting = &g_uart_qe_baud_setting, .flow_control =
          SCI_UART_FLOW_CONTROL_RTS,
#if 0xFF != 0xFF
                .flow_control_pin       = BSP_IO_PORT_FF_PIN_0xFF,
                #else
  .flow_control_pin = (bsp_io_port_pin_t) UINT16_MAX,
#endif
  .rs485_setting =
  { .enable = SCI_UART_RS485_DISABLE, .polarity = SCI_UART_RS485_DE_POLARITY_HIGH,
#if 0xFF != 0xFF
                    .de_control_pin = BSP_IO_PORT_FF_PIN_0xFF,
                #else
    .de_control_pin = (bsp_io_port_pin_t) UINT16_MAX,
#endif
          },
  .irda_setting =
  { .ircr_bits_b.ire = 0, .ircr_bits_b.irrxinv = 0, .ircr_bits_b.irtxinv = 0, }, };

/** UART interface configuration */
const uart_cfg_t g_uart_qe_cfg =
{ .channel = 9, .data_bits = UART_DATA_BITS_8, .parity = UART_PARITY_OFF, .stop_bits = UART_STOP_BITS_1, .p_callback =
          touch_uart_callback,
  .p_context = NULL, .p_extend = &g_uart_qe_cfg_extend,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
  .p_transfer_tx = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
  .p_transfer_rx = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
  .rxi_ipl = (2),
  .txi_ipl = (2), .tei_ipl = (2), .eri_ipl = (2),
#if defined(VECTOR_NUMBER_SCI9_RXI)
                .rxi_irq             = VECTOR_NUMBER_SCI9_RXI,
#else
  .rxi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI9_TXI)
                .txi_irq             = VECTOR_NUMBER_SCI9_TXI,
#else
  .txi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI9_TEI)
                .tei_irq             = VECTOR_NUMBER_SCI9_TEI,
#else
  .tei_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI9_ERI)
                .eri_irq             = VECTOR_NUMBER_SCI9_ERI,
#else
  .eri_irq = FSP_INVALID_VECTOR,
#endif
        };

/* Instance structure to use this module. */
const uart_instance_t g_uart_qe =
{ .p_ctrl = &g_uart_qe_ctrl, .p_cfg = &g_uart_qe_cfg, .p_api = &g_uart_on_sci };
dtc_instance_ctrl_t g_transfer1_ctrl;

#if (BSP_CFG_DCACHE_ENABLED) && (1 == 1)
const transfer_info_t g_transfer1_user_config_info =
{
    .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
    .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_DESTINATION,
    .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
    .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
    .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_FIXED,
    .transfer_settings_word_b.size           = TRANSFER_SIZE_2_BYTE,
    .transfer_settings_word_b.mode           = TRANSFER_MODE_NORMAL,
    .p_dest                                  = (void *) NULL,
    .p_src                                   = (void const *) NULL,
    .num_blocks                              = (uint16_t) 0,
    .length                                  = (uint16_t) 0,
};
#endif

#if BSP_CFG_DCACHE_ENABLED
    #if (1 > 0)
    transfer_info_t g_transfer1_info_fsp_nocache[1] DTC_TRANSFER_INFO_ALIGNMENT;
    #else
    /* User must call api::reconfigure before enable DTC transfer. */
    #endif
#else
#if (1 == 1)
transfer_info_t g_transfer1_info DTC_TRANSFER_INFO_ALIGNMENT =
{ .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
  .transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_DESTINATION,
  .transfer_settings_word_b.irq = TRANSFER_IRQ_END,
  .transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
  .transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED,
  .transfer_settings_word_b.size = TRANSFER_SIZE_2_BYTE,
  .transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,
  .p_dest = (void*) NULL,
  .p_src = (void const*) NULL,
  .num_blocks = (uint16_t) 0,
  .length = (uint16_t) 0, };
#elif (1 > 1)
    /* User is responsible to initialize the array. */
    transfer_info_t g_transfer1_info[1] DTC_TRANSFER_INFO_ALIGNMENT;
    #else
    /* User must call api::reconfigure before enable DTC transfer. */
    #endif
#endif

const dtc_extended_cfg_t g_transfer1_cfg_extend =
{ .activation_source = VECTOR_NUMBER_CTSU_READ,

#if BSP_CFG_DCACHE_ENABLED
    #if (1 == 1)
        .p_user_config_info =  &g_transfer1_user_config_info,
    #else
        .p_user_config_info = NULL,
    #endif
#else
        /* p_user_config_info not present. */
#endif
        };

const transfer_cfg_t g_transfer1_cfg =
{
#if BSP_CFG_DCACHE_ENABLED
    #if (1 > 0)
        .p_info              = g_transfer1_info_fsp_nocache,
    #else
        .p_info = NULL,
    #endif
#else
#if (1 == 1)
  .p_info = &g_transfer1_info,
#elif (1 > 1)
        .p_info              = g_transfer1_info,
    #else
        .p_info = NULL,
    #endif
#endif
  .p_extend = &g_transfer1_cfg_extend, };

/* Instance structure to use this module. */
const transfer_instance_t g_transfer1 =
{ .p_ctrl = &g_transfer1_ctrl, .p_cfg = &g_transfer1_cfg, .p_api = &g_transfer_on_dtc };
dtc_instance_ctrl_t g_transfer0_ctrl;

#if (BSP_CFG_DCACHE_ENABLED) && (1 == 1)
const transfer_info_t g_transfer0_user_config_info =
{
    .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
    .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_SOURCE,
    .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
    .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
    .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_INCREMENTED,
    .transfer_settings_word_b.size           = TRANSFER_SIZE_2_BYTE,
    .transfer_settings_word_b.mode           = TRANSFER_MODE_NORMAL,
    .p_dest                                  = (void *) NULL,
    .p_src                                   = (void const *) NULL,
    .num_blocks                              = (uint16_t) 0,
    .length                                  = (uint16_t) 0,
};
#endif

#if BSP_CFG_DCACHE_ENABLED
    #if (1 > 0)
    transfer_info_t g_transfer0_info_fsp_nocache[1] DTC_TRANSFER_INFO_ALIGNMENT;
    #else
    /* User must call api::reconfigure before enable DTC transfer. */
    #endif
#else
#if (1 == 1)
transfer_info_t g_transfer0_info DTC_TRANSFER_INFO_ALIGNMENT =
{ .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
  .transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
  .transfer_settings_word_b.irq = TRANSFER_IRQ_END,
  .transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
  .transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
  .transfer_settings_word_b.size = TRANSFER_SIZE_2_BYTE,
  .transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,
  .p_dest = (void*) NULL,
  .p_src = (void const*) NULL,
  .num_blocks = (uint16_t) 0,
  .length = (uint16_t) 0, };
#elif (1 > 1)
    /* User is responsible to initialize the array. */
    transfer_info_t g_transfer0_info[1] DTC_TRANSFER_INFO_ALIGNMENT;
    #else
    /* User must call api::reconfigure before enable DTC transfer. */
    #endif
#endif

const dtc_extended_cfg_t g_transfer0_cfg_extend =
{ .activation_source = VECTOR_NUMBER_CTSU_WRITE,

#if BSP_CFG_DCACHE_ENABLED
    #if (1 == 1)
        .p_user_config_info =  &g_transfer0_user_config_info,
    #else
        .p_user_config_info = NULL,
    #endif
#else
        /* p_user_config_info not present. */
#endif
        };

const transfer_cfg_t g_transfer0_cfg =
{
#if BSP_CFG_DCACHE_ENABLED
    #if (1 > 0)
        .p_info              = g_transfer0_info_fsp_nocache,
    #else
        .p_info = NULL,
    #endif
#else
#if (1 == 1)
  .p_info = &g_transfer0_info,
#elif (1 > 1)
        .p_info              = g_transfer0_info,
    #else
        .p_info = NULL,
    #endif
#endif
  .p_extend = &g_transfer0_cfg_extend, };

/* Instance structure to use this module. */
const transfer_instance_t g_transfer0 =
{ .p_ctrl = &g_transfer0_ctrl, .p_cfg = &g_transfer0_cfg, .p_api = &g_transfer_on_dtc };
icu_instance_ctrl_t g_external_irq0_ctrl;

/** External IRQ extended configuration for ICU HAL driver */
const icu_extended_cfg_t g_external_irq0_ext_cfg =
{ .filter_src = EXTERNAL_IRQ_DIGITAL_FILTER_PCLK_DIV, };

const external_irq_cfg_t g_external_irq0_cfg =
{ .channel = 0, .trigger = EXTERNAL_IRQ_TRIG_RISING, .filter_enable = false, .clock_source_div =
          EXTERNAL_IRQ_CLOCK_SOURCE_DIV_64,
  .p_callback = NULL,
  /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
  .p_context = (void*) &NULL,
#endif
  .p_extend = (void*) &g_external_irq0_ext_cfg,
  .ipl = (2),
#if defined(VECTOR_NUMBER_ICU_IRQ0)
    .irq                 = VECTOR_NUMBER_ICU_IRQ0,
#else
  .irq = FSP_INVALID_VECTOR,
#endif
        };
/* Instance structure to use this module. */
const external_irq_instance_t g_external_irq0 =
{ .p_ctrl = &g_external_irq0_ctrl, .p_cfg = &g_external_irq0_cfg, .p_api = &g_external_irq_on_icu };
ioport_instance_ctrl_t g_ioport_ctrl;
const ioport_instance_t g_ioport =
{ .p_api = &g_ioport_on_ioport, .p_ctrl = &g_ioport_ctrl, .p_cfg = &g_bsp_pin_cfg, };
EventGroupHandle_t g_heartbeat_event_group;
#if 1
StaticEventGroup_t g_heartbeat_event_group_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
EventGroupHandle_t g_system_state_event_group;
#if 1
StaticEventGroup_t g_system_state_event_group_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
SemaphoreHandle_t g_eeprom_mutex;
#if 1
StaticSemaphore_t g_eeprom_mutex_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
SemaphoreHandle_t g_config_mutex;
#if 1
StaticSemaphore_t g_config_mutex_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
SemaphoreHandle_t g_ir_capture_semaphore;
#if 1
StaticSemaphore_t g_ir_capture_semaphore_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
SemaphoreHandle_t g_uart_rx_semaphore;
#if 1
StaticSemaphore_t g_uart_rx_semaphore_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
SemaphoreHandle_t g_ctsu_scan_complete_semaphore;
#if 1
StaticSemaphore_t g_ctsu_scan_complete_semaphore_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
SemaphoreHandle_t g_touch_scan_complete_semaphore;
#if 1
StaticSemaphore_t g_touch_scan_complete_semaphore_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
SemaphoreHandle_t g_flash_op_complete_semaphore;
#if 1
StaticSemaphore_t g_flash_op_complete_semaphore_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
SemaphoreHandle_t g_uart_tx_complete_semaphore;
#if 1
StaticSemaphore_t g_uart_tx_complete_semaphore_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
void g_common_init(void)
{
    g_heartbeat_event_group =
#if 1
            xEventGroupCreateStatic (&g_heartbeat_event_group_memory);
#else
                xEventGroupCreate();
                #endif
    if (NULL == g_heartbeat_event_group)
    {
        rtos_startup_err_callback (g_heartbeat_event_group, 0);
    }
    g_system_state_event_group =
#if 1
            xEventGroupCreateStatic (&g_system_state_event_group_memory);
#else
                xEventGroupCreate();
                #endif
    if (NULL == g_system_state_event_group)
    {
        rtos_startup_err_callback (g_system_state_event_group, 0);
    }
    g_eeprom_mutex =
#if 0
                #if 1
                xSemaphoreCreateRecursiveMutexStatic(&g_eeprom_mutex_memory);
                #else
                xSemaphoreCreateRecursiveMutex();
                #endif
                #else
#if 1
            xSemaphoreCreateMutexStatic (&g_eeprom_mutex_memory);
#else
                xSemaphoreCreateMutex();
                #endif
#endif
    if (NULL == g_eeprom_mutex)
    {
        rtos_startup_err_callback (g_eeprom_mutex, 0);
    }
    g_config_mutex =
#if 0
                #if 1
                xSemaphoreCreateRecursiveMutexStatic(&g_config_mutex_memory);
                #else
                xSemaphoreCreateRecursiveMutex();
                #endif
                #else
#if 1
            xSemaphoreCreateMutexStatic (&g_config_mutex_memory);
#else
                xSemaphoreCreateMutex();
                #endif
#endif
    if (NULL == g_config_mutex)
    {
        rtos_startup_err_callback (g_config_mutex, 0);
    }
    g_ir_capture_semaphore =
#if 1
            xSemaphoreCreateBinaryStatic (&g_ir_capture_semaphore_memory);
#else
                xSemaphoreCreateBinary();
                #endif
    if (NULL == g_ir_capture_semaphore)
    {
        rtos_startup_err_callback (g_ir_capture_semaphore, 0);
    }
    g_uart_rx_semaphore =
#if 1
            xSemaphoreCreateBinaryStatic (&g_uart_rx_semaphore_memory);
#else
                xSemaphoreCreateBinary();
                #endif
    if (NULL == g_uart_rx_semaphore)
    {
        rtos_startup_err_callback (g_uart_rx_semaphore, 0);
    }
    g_ctsu_scan_complete_semaphore =
#if 1
            xSemaphoreCreateBinaryStatic (&g_ctsu_scan_complete_semaphore_memory);
#else
                xSemaphoreCreateBinary();
                #endif
    if (NULL == g_ctsu_scan_complete_semaphore)
    {
        rtos_startup_err_callback (g_ctsu_scan_complete_semaphore, 0);
    }
    g_touch_scan_complete_semaphore =
#if 1
            xSemaphoreCreateBinaryStatic (&g_touch_scan_complete_semaphore_memory);
#else
                xSemaphoreCreateBinary();
                #endif
    if (NULL == g_touch_scan_complete_semaphore)
    {
        rtos_startup_err_callback (g_touch_scan_complete_semaphore, 0);
    }
    g_flash_op_complete_semaphore =
#if 1
            xSemaphoreCreateBinaryStatic (&g_flash_op_complete_semaphore_memory);
#else
                xSemaphoreCreateBinary();
                #endif
    if (NULL == g_flash_op_complete_semaphore)
    {
        rtos_startup_err_callback (g_flash_op_complete_semaphore, 0);
    }
    g_uart_tx_complete_semaphore =
#if 1
            xSemaphoreCreateBinaryStatic (&g_uart_tx_complete_semaphore_memory);
#else
                xSemaphoreCreateBinary();
                #endif
    if (NULL == g_uart_tx_complete_semaphore)
    {
        rtos_startup_err_callback (g_uart_tx_complete_semaphore, 0);
    }
}
