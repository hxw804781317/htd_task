// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_FIFO_H_
#define _HTD_FIFO_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    const char* p_name;
    uint8_t *p_buff;           // 内存
    uint16_t buf_size_mask;
    volatile size_t read_pos;
    volatile size_t write_pos; 
}htd_fifo_t;

// @brief fifo 初始化
int32_t htd_fifo_init(htd_fifo_t *p_fifo, const char *p_name, uint8_t *p_buff, uint16_t buff_size);
// @brief fifo 清除所有数据
void htd_fifo_flush(htd_fifo_t *p_fifo);
// @brief 查看fifo数据长度
uint32_t htd_fifo_length(const htd_fifo_t *p_fifo);
// @brief fifo 有空余
uint32_t htd_fifo_remain_len(const htd_fifo_t *p_fifo);


// @brief 读取多长数据
// @return 成功返回读取到的数据长度
int32_t htd_fifo_read(htd_fifo_t *p_fifo, uint8_t *p_buff, uint32_t buff_size);
// @brief 捡出多长数据(队列的数据长度不变)
// @return 成功返回捡出到的数据长度
int32_t htd_fifo_peek(htd_fifo_t *p_fifo, uint8_t *p_buff, uint32_t buff_size);
// @brief 写入多长数据
// @return 成功返回写入的数据长度
int32_t htd_fifo_write(htd_fifo_t *p_fifo, uint8_t const *p_buff, uint32_t buff_len);


// @brief fifo 添加一个字节
int32_t htd_fifo_put(htd_fifo_t *p_fifo, uint8_t byte);
// @brief fifo 获取一个字节
int32_t htd_fifo_get(htd_fifo_t *p_fifo, uint8_t *p_byte);


#ifdef __cplusplus
}
#endif /*__FIFO_H__*/
#endif // _HTD_FIFO_H_

