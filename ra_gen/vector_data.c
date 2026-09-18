/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = sci_uart_rxi_isr, /* SCI0 RXI (Receive data full) */
            [1] = sci_uart_txi_isr, /* SCI0 TXI (Transmit data empty) */
            [2] = sci_uart_tei_isr, /* SCI0 TEI (Transmit end) */
            [3] = agt_int_isr, /* AGT0 INT (AGT interrupt) */
            [4] = gpt_capture_compare_a_isr, /* GPT7 CAPTURE COMPARE A (Capture/Compare match A) */
            [5] = gpt_capture_compare_b_isr, /* GPT7 CAPTURE COMPARE B (Capture/Compare match B) */
            [6] = sci_uart_tei_isr, /* SCI9 TEI (Transmit end) */
            [7] = sci_uart_eri_isr, /* SCI9 ERI (Receive error) */
            [8] = r_icu_isr, /* ICU IRQ0 (External pin interrupt 0) */
            [9] = ctsu_write_isr, /* CTSU WRITE (Write request interrupt) */
            [10] = fcu_frdyi_isr, /* FCU FRDYI (Flash ready interrupt) */
            [11] = sci_uart_eri_isr, /* SCI0 ERI (Receive error) */
            [12] = sci_uart_rxi_isr, /* SCI9 RXI (Receive data full) */
            [13] = sci_uart_txi_isr, /* SCI9 TXI (Transmit data empty) */
            [14] = ctsu_read_isr, /* CTSU READ (Measurement data transfer request interrupt) */
            [15] = ctsu_end_isr, /* CTSU END (Measurement end interrupt) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_SCI0_RXI,GROUP0), /* SCI0 RXI (Receive data full) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_SCI0_TXI,GROUP1), /* SCI0 TXI (Transmit data empty) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_SCI0_TEI,GROUP2), /* SCI0 TEI (Transmit end) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_AGT0_INT,GROUP3), /* AGT0 INT (AGT interrupt) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_GPT7_CAPTURE_COMPARE_A,GROUP4), /* GPT7 CAPTURE COMPARE A (Capture/Compare match A) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_GPT7_CAPTURE_COMPARE_B,GROUP5), /* GPT7 CAPTURE COMPARE B (Capture/Compare match B) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TEI,GROUP6), /* SCI9 TEI (Transmit end) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_SCI9_ERI,GROUP7), /* SCI9 ERI (Receive error) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_ICU_IRQ0,GROUP0), /* ICU IRQ0 (External pin interrupt 0) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_CTSU_WRITE,GROUP1), /* CTSU WRITE (Write request interrupt) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_FCU_FRDYI,GROUP2), /* FCU FRDYI (Flash ready interrupt) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_SCI0_ERI,GROUP3), /* SCI0 ERI (Receive error) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_SCI9_RXI,GROUP4), /* SCI9 RXI (Receive data full) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TXI,GROUP5), /* SCI9 TXI (Transmit data empty) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_CTSU_READ,GROUP6), /* CTSU READ (Measurement data transfer request interrupt) */
            [15] = BSP_PRV_VECT_ENUM(EVENT_CTSU_END,GROUP7), /* CTSU END (Measurement end interrupt) */
        };
        #endif
        #endif
