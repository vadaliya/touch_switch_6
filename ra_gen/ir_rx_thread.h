/* generated thread header file - do not edit */
#ifndef IR_RX_THREAD_H_
#define IR_RX_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void ir_rx_thread_entry(void * pvParameters);
                #else
extern void ir_rx_thread_entry(void *pvParameters);
#endif
FSP_HEADER
FSP_FOOTER
#endif /* IR_RX_THREAD_H_ */
