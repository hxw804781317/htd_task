// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BOOT_DECOMPRESS_H_
#define _HTD_BOOT_DECOMPRESS_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "htd_boot_port.h"
#include "htd_boot_process.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

// 解压缩配置
#define BOOT_DECOMPRESS_BUFFER_SIZE         4096
#define BOOT_COMPRESS_BUFFER_SIZE           4096
#define BOOT_COMPRESS_BUFFER_PADDING        400


// @brief quicklz 解压
bool boot_decompress_quicklz(htd_boot_object_t* p_object);

// @brief 解压
bool boot_decompress(htd_boot_object_t* p_object);

#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BOOT_DECOMPRESS_H_*/

