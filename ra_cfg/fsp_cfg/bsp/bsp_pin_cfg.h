/* generated configuration header file - do not edit */
#ifndef BSP_PIN_CFG_H_
#define BSP_PIN_CFG_H_
#include "r_ioport.h"

/* Common macro for FSP header files. There is also a corresponding FSP_FOOTER macro at the end of this file. */
FSP_HEADER

#define S1 (BSP_IO_PORT_00_PIN_00)
#define S2 (BSP_IO_PORT_00_PIN_01)
#define S3 (BSP_IO_PORT_00_PIN_02)
#define S4 (BSP_IO_PORT_00_PIN_10)
#define S5 (BSP_IO_PORT_00_PIN_11)
#define S6 (BSP_IO_PORT_00_PIN_12)
#define F1 (BSP_IO_PORT_00_PIN_13)
#define Slider_Step_1 (BSP_IO_PORT_00_PIN_15)
#define Slider_Step_2 (BSP_IO_PORT_01_PIN_02)
#define Slider_Step_3 (BSP_IO_PORT_01_PIN_03)
#define Slider_Step_4 (BSP_IO_PORT_01_PIN_04)

extern const ioport_cfg_t g_bsp_pin_cfg; /* R7FA2E1A72DFL.pincfg */

void BSP_PinConfigSecurityInit();

/* Common macro for FSP header files. There is also a corresponding FSP_HEADER macro at the top of this file. */
FSP_FOOTER
#endif /* BSP_PIN_CFG_H_ */
