#ifndef _HTD_CONFIG_H__
#define _HTD_CONFIG_H__

#define SOC_FAMILY_STM32 1
#define SOC_SERIES_STM32F1 1
#define SOC_SERIES_STM32F103XX 1
#define SOC_STM32F103XB 1

/* HTD-TASK */

#define HTD_TASK_ENABLE 1
#define HTD_TASK_NONOS 1
#define HTD_MEMHEAP_FF_ENABLE 1
#define HTD_MEMHEAP_SRAM_DEV_COUNT 2
/* end of HTD-TASK */
#define ARCH_ARM 1
#define ARCH_ARM_CORTEX_M 1
#define ARCH_ARM_CORTEX_M3 1

/* components config */

#define HTD_CONSOLE_ENABLE 1
#define HTD_CONSOLE_NAME "htd"
#define HTD_CONSOLE_SUPPORT_CMD 1
#define HTD_CONSOLE_SUPPORT_CMD_HISTORY 1
#define HTD_CONSOLE_HISTORY_LINES 5
#define HTD_CONSOLE_CMD_SIZE 48
#define HTD_CONSOLE_SERIAL_NAME "segger_rtt"
#define HTD_CONSOLE_SERIAL_TX_PINNUM 0x09
#define HTD_CONSOLE_SERIAL_RX_PINNUM 0x0A
#define HTD_CONSOLE_SERIAL_BAUDRATE 115200
#define HTD_BUTTON_ENABLE 1
#define HTD_BUTTON_SUPPORT_MATRIX 1
#define BUTTON_MATRIX_SUPPORT_REPEAT 1
#define HTD_LED_ENABLE 1
#define HTD_LED_SUPPORT_CHARLIE 1
#define HTD_LED_USE_HW_TIMER 1
#define HTD_LED_HW_TIMER_NAME "timer1"
/* end of components config */

/* components config */

#define HTD_LIB_USE_SEGGER_RTT 1
/* end of components config */

/* On-chip Peripheral Drivers */

#define BSP_USING_UART 1
#define BSP_USING_RTC 1
#define BSP_USING_PM 1
#define BSP_USING_CRC 1
/* end of On-chip Peripheral Drivers */
#include "htd_config_project.h"

#endif
