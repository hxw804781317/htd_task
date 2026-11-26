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
/* HTD_BOOT_ENABLE is not set */
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
#define HTD_FLASH_UTILITY_BOX_ENABLE 1
#define HTD_FLASH_KV_ENABLE 1
#define HTD_FLASH_BOX_ENABLE 1
#define HTD_FLASH_LOG_ENABLE 1
#define HTD_FLASH_VAR_ENABLE 1
#define HTD_VFS_ENABLE 1
#define HTD_VFS_USING_NORFLASH_DEV 1
/* HTD_VFS_USING_NANDFLASH_DEV is not set */
#define HTD_VFS_USING_LIB_DHARA 1
/* HTD_VFS_USING_SDCARD_DEV is not set */
#define HTD_VFS_USING_POSIX 1
#define HTD_VFS_FD_MAX 16
/* HTD_VFS_USING_ROMFS is not set */
#define HTD_VFS_USING_RAMFS 1
/* HTD_VFS_USING_SPIFFS is not set */
#define HTD_VFS_USING_ELM_FATFS 1

/* elm-chan's FatFs, Generic FAT Filesystem Module */
#define VFS_ELM_FATFS_CODE_PAGE 437
#define VFS_ELM_FATFS_WORD_ACCESS 1
/* VFS_ELM_FATFS_USE_LFN_0 is not set */
/* VFS_ELM_FATFS_USE_LFN_1 is not set */
/* VFS_ELM_FATFS_USE_LFN_2 is not set */
#define VFS_ELM_FATFS_USE_LFN_3 1
#define VFS_ELM_FATFS_USE_LFN 3
#define VFS_ELM_FATFS_MAX_LFN 255
#define VFS_ELM_FATFS_LFN_UNICODE_0 1
/* VFS_ELM_FATFS_LFN_UNICODE_1 is not set */
/* VFS_ELM_FATFS_LFN_UNICODE_2 is not set */
/* VFS_ELM_FATFS_LFN_UNICODE_3 is not set */
#define VFS_ELM_FATFS_LFN_UNICODE 0
#define VFS_ELM_FATFS_DRIVES 1
#define VFS_ELM_FATFS_REENTRANT 1
/* end of elm-chan's FatFs, Generic FAT Filesystem Module */

/* HTD_VFS_USING_LITTLE_FS is not set */
/* end of components config */

/* Hardware Drivers Config */
/* HTD_USING_NANDFLASH is not set */
#define HTD_USING_NORFLASH 1
#define HTD_NORFLASH_START_ADDR 0x12000000
#define HTD_NORFLASH_CAPACITY 32
#define HTD_NORFLASH_BUS_NAME "spim1"
#define HTD_NORFLASH_POWER_PIN -1
#define HTD_NORFLASH_PIN_CS -1
#define HTD_NORFLASH_PIN_SCK -1
#define HTD_NORFLASH_PIN_MOSI -1
#define HTD_NORFLASH_PIN_MISO -1
/* HTD_USING_EEPROM is not set */
/* end of Hardware Drivers Config */

/* external config */
/* HTD_LIB_USE_EASYTLV is not set */
/* HTD_LIB_USE_MICRO_ECC is not set */
/* HTD_LIB_USE_QUICKLZ is not set */
/* HTD_LIB_USE_FASTLZ is not set */
#define HTD_LIB_USE_SDL2 1
/* HTD_LIB_USE_SEGGER_RTT is not set */
/* HTD_LIB_USE_FREERTOS is not set */
/* end of external config */

#include "htd_config_project.h"

#endif /*_HTD_CONFIG_H_*/
