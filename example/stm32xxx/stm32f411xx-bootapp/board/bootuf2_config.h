/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BOOTUF2_CONFIG_H
#define BOOTUF2_CONFIG_H

#define CONFIG_PRODUCT                      "py32f403"
#define CONFIG_BOARD                        "PY32F403 BOARD"

#define CONFIG_BOOTUF2_CACHE_SIZE           4096
#define CONFIG_BOOTUF2_SECTOR_SIZE          512
#define CONFIG_BOOTUF2_SECTOR_PER_CLUSTER   1
#define CONFIG_BOOTUF2_SECTOR_RESERVED      1
#define CONFIG_BOOTUF2_NUM_OF_FAT           2
#define CONFIG_BOOTUF2_ROOT_ENTRIES         64

#define CONFIG_BOOTUF2_FAMILYID             0x7d7a66f0
#define CONFIG_BOOTUF2_FLASHMAX             0x800000
#define CONFIG_BOOTUF2_PAGE_COUNTMAX        1024

#define CONFIG_BOOTUF2_FILE_COUNT           2
#define CONFIG_BOOTUF2_START_ADDR           0x8000000

#endif
