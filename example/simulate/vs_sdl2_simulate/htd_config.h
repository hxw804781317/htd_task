#ifndef _HTD_CONFIG_H__
#define _HTD_CONFIG_H__

#define SOC_SIMULATOR 1
#define BSP_USING_PC_SIMULATOR 1

/* HTD-TASK */

#define HTD_TASK_ENABLE 1
#define HTD_TASK_WIN32_SDL2 1
#define HTD_TASK_THREAD_PRIORITY 28
#define HTD_TASK_THREAD_STACK_SIZE 4096
#define HTD_MEMHEAP_FF_ENABLE 1
#define HTD_MEMHEAP_SRAM_DEV_COUNT 2
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
/* end of components config */

/* external config */

#define HTD_LIB_USE_SDL2 1
/* end of external config */
#include "htd_config_project.h"

#endif
