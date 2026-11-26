// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_FLASH_VAR_H_
#define _HTD_FLASH_VAR_H_

#include "htd_flash_common.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// 最大的变量大小
#ifndef FLASHVAR_MAX_SIZE
    #define FLASHVAR_MAX_SIZE       256   // TODO: 需要根据需求调整
#endif


// 参数 flash 信息
typedef struct {
    uint32_t init_flag;                 // 初始化标志
    uint16_t index;
    uint16_t var_len;                   // 最后参数大小
    uint32_t var_addr;                  // 最后写入的地址
    const flash_config_t* p_cfg;
    const flash_handle_t* p_flash;
}flashvar_t;


// @brief 初始化参数
int32_t htd_flashvar_init(flashvar_t* p_var, const flash_config_t* p_cfg);
// @brief 读取变量值, 成功返回读取到的数据长度，失败返回对应的错误码(负值)
int32_t htd_flashvar_get(flashvar_t* p_var, void* p_buff, uint16_t buff_size);
// @brief 更新变量值, 成功返回写入的数据长度，失败返回对应的错误码(负值)
int32_t htd_flashvar_set(flashvar_t* p_var, const void* p_buff, uint16_t buff_len);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_FLASH_VAR_H_*/

