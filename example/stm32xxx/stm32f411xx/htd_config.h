#ifndef _HTD_CONFIG_H_
#define _HTD_CONFIG_H_

#define SOC_FAMILY_STM32 1
#define SOC_SERIES_STM32F4 1
#define SOC_SERIES_STM32F411XX 1
#define SOC_STM32F411xE 1

/* HTD-TASK */
#define HTD_TASK_ENABLE 1
#define HTD_TASK_NONOS 1
/* HTD_TASK_FREERTOS is not set */
#define HTD_MEMHEAP_FF_ENABLE 1
#define HTD_MEMHEAP_SRAM_DEV_COUNT 2
/* HTD_MEMHEAP_FF_USE_TRACE is not set */
/* HTD_PRINTF_SUPPORT_FLOAT is not set */
/* HTD_MEM_USE_BYTELOOP is not set */
/* HTD_VER_RELEASE is not set */
/* end of HTD-TASK */

#define ARCH_ARM 1
#define ARCH_ARM_CORTEX_M 1
#define ARCH_ARM_CORTEX_FPU 1
#define ARCH_ARM_CORTEX_M4 1

/* components config */
/* HTD_BOOT_ENABLE is not set */
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
#define HTD_BUTTON_SUPPORT_CHARLIE 1
#define BUTTON_CHARLIE_SUPPORT_REPEAT 1
#define HTD_LED_ENABLE 1
/* HTD_LED_SUPPORT_CHARLIE is not set */
/* HTD_LED_SUPPORT_MATRIX is not set */
/* HTD_LED_USE_HW_TIMER is not set */
/* end of components config */

/* external config */
/* HTD_LIB_USE_EASYTLV is not set */
/* HTD_LIB_USE_MICRO_ECC is not set */
/* HTD_LIB_USE_QUICKLZ is not set */
/* HTD_LIB_USE_FASTLZ is not set */
/* HTD_LIB_USE_SDL2 is not set */
#define HTD_LIB_USE_SEGGER_RTT 1
/* HTD_LIB_USE_FREERTOS is not set */
/* end of external config */

/* On-chip Peripheral Drivers */
#define BSP_USING_UART 1
#define BSP_USING_UART1 1
#define BSP_UART1_RX_USING_DMA 1
#define BSP_UART1_TX_USING_DMA 1
/* BSP_USING_UART2 is not set */
/* BSP_USING_UART3 is not set */
/* BSP_USING_UART4 is not set */
/* BSP_USING_UART5 is not set */
/* BSP_USING_UART6 is not set */
#define BSP_USING_SPIM 1
#define BSP_USING_SPIM1 1
#define BSP_SPIM1_USING_DMA 1
/* BSP_USING_SPIM2 is not set */
/* BSP_USING_SPIM3 is not set */
/* BSP_USING_SPIM4 is not set */
/* BSP_USING_SPIM5 is not set */
#define BSP_USING_SFW_SPIM1 1
#define BSP_USING_SFW_SPIM2 1
/* BSP_USING_SFW_SPIM3 is not set */
/* BSP_USING_SFW_SPIM4 is not set */
/* BSP_USING_SFW_SPIM5 is not set */
#define BSP_USING_I2CM 1
#define BSP_USING_I2CM1 1
/* BSP_USING_I2CM2 is not set */
/* BSP_USING_I2CM3 is not set */
#define BSP_USING_SFW_I2CM1 1
/* BSP_USING_SFW_I2CM2 is not set */
/* BSP_USING_SFW_I2CM3 is not set */
/* BSP_USING_SDIO is not set */
/* BSP_USING_ADC is not set */
#define BSP_USING_RTC 1
/* BSP_USING_WDT is not set */
#define BSP_USING_PM 1
#define BSP_USING_CRC 1
/* BSP_USING_USB_DEVICE is not set */
/* BSP_USING_USB_HOST is not set */
/* BSP_USING_TIM_PWM_OCC is not set */
/* end of On-chip Peripheral Drivers */

#include "htd_config_project.h"

#endif /*_HTD_CONFIG_H_*/
