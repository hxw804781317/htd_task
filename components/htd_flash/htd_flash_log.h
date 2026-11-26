// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_FLASH_LOG_H_
#define _HTD_FLASH_LOG_H_

#include "htd_flash_common.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// ring buffer 结构，不断写入会覆盖最先写入的
typedef struct {
    uint16_t init_flag;
    uint16_t busy_flag;
    uint32_t rpos;
    uint32_t wpos;
    uint32_t pidx;              // 最后的页序列号
    const flash_config_t* p_cfg;
    const flash_handle_t* p_flash;
} flashlog_t;


// @brief 初始化
int32_t htd_flashlog_init(flashlog_t* p_rngbuf, const flash_config_t* p_cfg);
// @brief 去初始化
int32_t htd_flashlog_uninit(flashlog_t* p_rngbuf);
// @brief 写入数据
int32_t htd_flashlog_write(flashlog_t* p_rngbuf, const uint8_t* p_buff, uint32_t buff_len);
// @brief 读取数据
int32_t htd_flashlog_read(flashlog_t* p_rngbuf, uint32_t offset, uint8_t* p_buff, uint32_t* buff_len);
// @brief 清除所有数据
void htd_flashlog_clear(flashlog_t* p_rngbuf);
// @brief 获取有效长度
uint32_t htd_flashlog_data_len(flashlog_t* p_rngbuf);
// @brief 打印接口
int32_t htd_flashlog_line_print(flashlog_t* p_rngbuf, const char *fmt, ...);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_FLASH_LOG_H_*/

