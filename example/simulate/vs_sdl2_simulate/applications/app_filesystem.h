// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef __APP_FILESYSTEM_H__
#define __APP_FILESYSTEM_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "htd_components.h"


#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief 初始化文件系统
int app_filesystem_init(void);
// @brief 挂载文件系统
int32_t app_filesystem_mount(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*__APP_FILESYSTEM_H__*/

