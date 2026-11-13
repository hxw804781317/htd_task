// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL     4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "fifo"
#include "htd_task.h"
#include "htd_fifo.h"

#define FIFO_INT_MAX            0xffffffff


// @brief fifo 初始化
int32_t htd_fifo_init(htd_fifo_t * p_fifo, const char* p_name, uint8_t * p_buff, uint16_t buff_size)
{
    ASSERT(p_fifo);
    ASSERT(p_buff);
    ASSERT(buff_size);
    
    // 是否为 2 的指数倍
    if (!IS_POWER_OF_TWO(buff_size)) {
        print_error("param error. %s, buff_size=%d", p_fifo->p_name, buff_size);
        ASSERT(0);
        return -1;
    }
    p_fifo->p_name = p_name;
    p_fifo->p_buff = p_buff;
    p_fifo->buf_size_mask = buff_size - 1;
    p_fifo->read_pos = 0;
    p_fifo->write_pos = 0;
    return 0;
}

// @brief fifo 清除所有数据
void htd_fifo_flush(htd_fifo_t * p_fifo)
{
    if (p_fifo) {
        p_fifo->read_pos = p_fifo->write_pos = 0;
    }
}

// @brief 查看fifo数据长度
uint32_t htd_fifo_length(const htd_fifo_t* p_fifo)
{
    if (p_fifo) {
        size_t rpos = p_fifo->read_pos;
        size_t wpos = p_fifo->write_pos;
        return (wpos - rpos) & FIFO_INT_MAX;
    }
    return 0;
}

// @brief fifo 有空余
uint32_t htd_fifo_remain_len(const htd_fifo_t* p_fifo)
{
    if (p_fifo) {
        size_t rpos = p_fifo->read_pos;
        size_t wpos = p_fifo->write_pos;
        size_t length = (wpos - rpos) & FIFO_INT_MAX;
        return (p_fifo->buf_size_mask + 1 - length);
    }
    return 0;
}

// @brief 读取多长数据
// @return 成功返回读取到的数据长度
int32_t __fifo_read(htd_fifo_t * p_fifo, uint8_t *p_buff, uint32_t buff_size, bool is_read)
{
    if (p_fifo == NULL || p_buff == NULL || buff_size == 0) {
        return 0;
    }

    size_t wpos = p_fifo->write_pos;
    size_t rpos = p_fifo->read_pos;
    if (rpos >= wpos) {
        print_error("name=%s, not data to read. rpos=%u, wpos=%u", p_fifo->p_name, rpos, wpos);
        return 0;
    }
    
    size_t front;
    size_t data_total = p_fifo->buf_size_mask + 1;
    size_t length = (wpos - rpos) & FIFO_INT_MAX;
    buff_size = MIN(length, buff_size);
    rpos = rpos & p_fifo->buf_size_mask;
    if (rpos + buff_size > data_total) {
        front = data_total - rpos;
        htd_memcpy(p_buff, (uint8_t*)p_fifo->p_buff + rpos, front);
        p_buff = (uint8_t*)p_buff + front;
        htd_memcpy(p_buff, p_fifo->p_buff, buff_size - front);
    } else {
        htd_memcpy(p_buff, (uint8_t*)p_fifo->p_buff + rpos, buff_size);
    }
    if (is_read) {
        p_fifo->read_pos += buff_size;
    }
    return buff_size;
}

// @brief 读取多长数据
// @return 成功返回读取到的数据长度
int32_t htd_fifo_read(htd_fifo_t * p_fifo, uint8_t *p_buff, uint32_t buff_size)
{
    return __fifo_read(p_fifo, p_buff, buff_size, true);
}

// @brief 捡出多长数据(队列的数据长度不变)
// @return 成功返回捡出到的数据长度
int32_t htd_fifo_peek(htd_fifo_t * p_fifo, uint8_t *p_buff, uint32_t buff_size)
{
    return __fifo_read(p_fifo, p_buff, buff_size, false);
}

// @brief 写入多长数据
// @return 成功返回写入的数据长度
int32_t htd_fifo_write(htd_fifo_t * p_fifo, uint8_t const * p_buff, uint32_t buff_len)
{
    ASSERT(p_fifo);
    ASSERT(buff_len <= p_fifo->buf_size_mask + 1UL);
    if (p_buff == NULL || buff_len == 0) {
        return 0;
    }

    size_t rpos = p_fifo->read_pos;
    size_t wpos = p_fifo->write_pos;
    size_t length = (wpos - rpos) & FIFO_INT_MAX;
    size_t data_total = p_fifo->buf_size_mask + 1;
    size_t remain_len = data_total - length;
    if (buff_len > remain_len) {
        print_warning("not enough space to write. remain_len=%d, buff_len=%d", remain_len, buff_len);
        buff_len = remain_len;
    }
    if (buff_len == 0) {
        return 0;
    }

    size_t front;
    size_t pos = wpos & p_fifo->buf_size_mask;
    if (pos + buff_len > data_total) {
        front = data_total - pos;
        htd_memcpy(p_fifo->p_buff + pos, p_buff, front);
        p_buff = p_buff + front;
        htd_memcpy(p_fifo->p_buff, p_buff, buff_len - front);
    } else {
        htd_memcpy(p_fifo->p_buff + pos, p_buff, buff_len);
    }
    p_fifo->write_pos += buff_len;
    return buff_len;
}

// @brief fifo 添加一个字节
int32_t htd_fifo_put(htd_fifo_t *p_fifo, uint8_t byte)
{
    ASSERT(p_fifo);
    size_t rpos = p_fifo->read_pos;
    size_t wpos = p_fifo->write_pos;
    size_t length = (wpos - rpos) & FIFO_INT_MAX;
    
    if (length <= p_fifo->buf_size_mask) {
        p_fifo->p_buff[wpos++ & p_fifo->buf_size_mask] = byte;
        p_fifo->write_pos = wpos;
        return 1;
    }
    return 0;
}

// @brief fifo 获取一个字节
int32_t htd_fifo_get(htd_fifo_t *p_fifo, uint8_t *p_byte)
{
    ASSERT(p_fifo);
    size_t rpos = p_fifo->read_pos;
    size_t wpos = p_fifo->write_pos;
    size_t length = (wpos - rpos) & FIFO_INT_MAX;
    if (length) {
        *p_byte = p_fifo->p_buff[rpos++ & p_fifo->buf_size_mask];
        p_fifo->read_pos = rpos;
        return 1;
    }
    return 0;
}


