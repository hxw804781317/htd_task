#ifndef _HTD_CONFIG_H_
#define _HTD_CONFIG_H_

#define SOC_SIMULATOR 1
#define BSP_USING_PC_SIMULATOR 1

/* HTD-TASK */
#define HTD_TASK_ENABLE 1
#define HTD_TASK_WIN32_SDL2 1
#define HTD_TASK_THREAD_PRIORITY 28
#define HTD_TASK_THREAD_STACK_SIZE 4096
#define HTD_MEMHEAP_FF_ENABLE 1
#define HTD_MEMHEAP_SRAM_DEV_COUNT 2
/* HTD_MEMHEAP_FF_USE_TRACE is not set */
/* HTD_PRINTF_SUPPORT_FLOAT is not set */
/* HTD_MEM_USE_BYTELOOP is not set */
/* HTD_VER_RELEASE is not set */
/* end of HTD-TASK */

/* components config */
#define HTD_CONSOLE_ENABLE 1
#define HTD_CONSOLE_NAME "htd"
#define HTD_CONSOLE_SUPPORT_CMD 1
#define HTD_CONSOLE_SUPPORT_CMD_HISTORY 1
#define HTD_CONSOLE_HISTORY_LINES 5
#define HTD_CONSOLE_CMD_SIZE 48
#define HTD_CONSOLE_SERIAL_NAME "console"
#define HTD_CONSOLE_SERIAL_TX_PINNUM 0x09
#define HTD_CONSOLE_SERIAL_RX_PINNUM 0x0A
#define HTD_CONSOLE_SERIAL_BAUDRATE 115200
/* HTD_BUTTON_ENABLE is not set */
/* HTD_LED_ENABLE is not set */
/* end of components config */

/* external config */
/* HTD_LIB_USE_EASYTLV is not set */
/* HTD_LIB_USE_MICRO_ECC is not set */
#define HTD_LIB_USE_SDL2 1
/* HTD_LIB_USE_SEGGER_RTT is not set */
/* HTD_LIB_USE_FREERTOS is not set */
/* end of external config */

#include "htd_config_project.h"

#endif /*_HTD_CONFIG_H_*/
