#ifndef _HTD_CONFIG_H_
#define _HTD_CONFIG_H_

#define SOC_FAMILY_STM32 1
#define SOC_SERIES_STM32F2 1
#define SOC_SERIES_STM32F207XX 1
#define SOC_STM32F207XC 1

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
#define ARCH_ARM_CORTEX_M3 1

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
/* HTD_BUTTON_ENABLE is not set */
/* HTD_LED_ENABLE is not set */
/* HTD_FLASH_UTILITY_BOX_ENABLE is not set */
/* HTD_VFS_ENABLE is not set */
/* HTD_ELFDL_ENABLE is not set */
/* end of components config */

/* Hardware Drivers Config */
/* HTD_USING_NANDFLASH is not set */
#define HTD_USING_NORFLASH 1
#define HTD_NORFLASH_START_ADDR 0x12000000
#define HTD_NORFLASH_CAPACITY 32
#define HTD_NORFLASH_BUS_NAME "spim1"
#define HTD_NORFLASH_POWER_PIN -1
#define HTD_NORFLASH_PIN_CS 4
#define HTD_NORFLASH_PIN_SCK 5
#define HTD_NORFLASH_PIN_MOSI 7
#define HTD_NORFLASH_PIN_MISO 6
/* HTD_USING_EEPROM is not set */
/* HTD_SENSOR_USING_BAROMETER is not set */
/* HTD_SENSOR_USING_GSENSOR is not set */
#define HTD_6AXIS_BUS_I2C 1
#define HTD_SENSOR_USING_6AXIS 1
/* HTD_6AXIS_USING_SIMULATE is not set */
#define HTD_6AXIS_USING_MPU6050 1
#define HTD_6AXIS_BUS_NAME "i2cm1"
#define HTD_6AXIS_I2C_ADDR 0x68
#define HTD_6AXIS_I2C_PIN_SDA 23
#define HTD_6AXIS_I2C_PIN_SCK 22
#define HTD_6AXIS_SAMPLE_FREQ 50
#define HTD_6AXIS_INT_PIN -1
#define HTD_6AXIS_POWER_PIN -1
/* HTD_USING_PIXEL_LED is not set */
/* HTD_USING_SDCARD is not set */
/* HTD_USING_NETWORK_SENSOR is not set */
/* end of Hardware Drivers Config */

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
#define BSP_USING_SFW_SPIM1 1
/* BSP_USING_SFW_SPIM2 is not set */
/* BSP_USING_SFW_SPIM3 is not set */
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
#define BSP_USING_RNG 1
#define BSP_USING_CRC 1
/* BSP_USING_USB_DEVICE is not set */
/* BSP_USING_USB_HOST is not set */
/* BSP_USING_TIM_PWM_OCC is not set */
/* end of On-chip Peripheral Drivers */

#include "htd_config_project.h"

#endif /*_HTD_CONFIG_H_*/
