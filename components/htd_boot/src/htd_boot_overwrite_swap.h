// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BOOT_SWAP_H_
#define _HTD_BOOT_SWAP_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "htd_boot_port.h"
#include "htd_boot_process.h"


#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief 数据交换
void boot_overwrite(htd_boot_object_t* p_object);
// @brief 数据回滚，回滚都是从 B->A 搬数据
void boot_rollback(htd_boot_object_t* p_object);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BOOT_SWAP_H_*/

