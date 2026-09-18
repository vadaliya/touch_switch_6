/* generated configuration header file - do not edit */
#ifndef BSP_PIN_CFG_H_
#define BSP_PIN_CFG_H_
#include "r_ioport.h"

/* Common macro for FSP header files. There is also a corresponding FSP_FOOTER macro at the end of this file. */
FSP_HEADER

#define FSP_TOUCH_SW3 (BSP_IO_PORT_00_PIN_00)
#define FSP_TOUCH_SW1 (BSP_IO_PORT_00_PIN_01)
#define FSP_RELAY_SW1 (BSP_IO_PORT_00_PIN_02)
#define FSP_RELAY_SW2 (BSP_IO_PORT_00_PIN_10)
#define FSP_RELAY_SW3 (BSP_IO_PORT_00_PIN_11)
#define FSP_RELAY_SW4 (BSP_IO_PORT_00_PIN_12)
#define FSP_RELAY_SW5 (BSP_IO_PORT_00_PIN_13)
#define FSP_RELAY_SW6 (BSP_IO_PORT_00_PIN_15)
#define FSP_TOUCH_SW2 (BSP_IO_PORT_01_PIN_01)
#define FSP_TOUCH_SW4 (BSP_IO_PORT_01_PIN_03)
#define FSP_TOUCH_SW6 (BSP_IO_PORT_03_PIN_01)
#define FSP_RC5_IR (BSP_IO_PORT_03_PIN_02)
#define FSP_BACKLIGHT_DATA_PIN (BSP_IO_PORT_04_PIN_00)
#define FSP_WIFI_DATA_PIN (BSP_IO_PORT_04_PIN_01)
#define FSP_TOUCH_SW5 (BSP_IO_PORT_04_PIN_09)
#define FSP_RELAY_EN (BSP_IO_PORT_05_PIN_00)

extern const ioport_cfg_t g_bsp_pin_cfg; /* R7FA2E1A72DFL.pincfg */

void BSP_PinConfigSecurityInit();

/* Common macro for FSP header files. There is also a corresponding FSP_HEADER macro at the top of this file. */
FSP_FOOTER
#endif /* BSP_PIN_CFG_H_ */
