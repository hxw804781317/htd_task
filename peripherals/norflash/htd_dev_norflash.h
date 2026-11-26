// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_NORFLASH_H_
#define _HTD_DEV_NORFLASH_H_

#include <stdint.h>
#include <stdbool.h>
#include <htd_fifo.h>
#include <htd_device.h>
#include <htd_dev_spim.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
// norflash 绝对地址
#ifndef NORFLASH_MAP_START_ADDR
    #ifdef HTD_NORFLASH_START_ADDR
        #define NORFLASH_MAP_START_ADDR     HTD_NORFLASH_START_ADDR
    #else
        #define NORFLASH_MAP_START_ADDR     0x12000000
    #endif
#endif
// norflash 容量, 实际以读取到的参数为准
#ifndef NORFLASH_MAP_CAPACITY
    #ifdef HTD_NORFLASH_CAPACITY
        #define NORFLASH_MAP_CAPACITY       HTD_NORFLASH_CAPACITY
    #else
        #define NORFLASH_MAP_CAPACITY       (32*1024*1024)
    #endif
#endif



//////////////////////////////////////////////////////////////////////
#define NORFLASH_CMD_NONE           0
#define NORFLASH_CMD_SECTOR_ERASE   1   // @norflash_erase_param_t
#define NORFLASH_CMD_SIZE_GET       2   // @uint32_t
#define NORFLASH_CMD_POWER_DOWN     3   // 
#define NORFLASH_CMD_WAKE_UP        4   // 
#define NORFLASH_CMD_CLOSE          5
#define NORFLASH_CMD_SHOW_INFO      6   // 
typedef uint32_t norflash_cmd_t;

typedef struct {
    uint32_t addr;
    uint32_t size;
} norflash_erase_param_t;

typedef struct {
    const char* p_name;
    uint32_t erase_unit;            // 每次擦除单位，单位字节
    uint32_t page_size;             // 页大小
    uint32_t capacity;              // flash 容量，单位字节; 超过16M(0x1000000)使用4字节地址

    uint8_t mf_id;                  // 厂商id
    uint8_t type_id;                // 类型id
    uint8_t erase_cmd;              // 擦除命令
    uint8_t write_unit;             // 一般情况下都是4字节
    uint8_t write_cmd;              // 写命令

    int32_t (*init_handle)(void);
    int32_t (*write_handle)(uint32_t addr, const uint8_t* p_buff, uint32_t buff_len);
    int32_t (*read_handle)(uint32_t addr, uint8_t* p_buff, uint32_t buff_len);
    int32_t (*ctrl_handle)(norflash_cmd_t cmd, void* p_param, uint32_t param_len);
} dev_norflash_t;

// @brief 注册(只能调用一次)
int32_t htd_dev_norflash_register(const dev_norflash_t* p_norflash);
// @brief 展示已经挂载的norflash信息
void htd_dev_norflash_show(void);


// @brief 读数据(支持addr不用按4字节对齐，data_len不用按4字节对齐，读取长度可以超过一页)
int32_t htd_dev_norflash_read(uint32_t addr, uint8_t* p_data, uint32_t data_len);
// @brief 写数据(支持addr不用按4字节对齐，data_len不用按4字节对齐，写入长度可以超过一页)
int32_t htd_dev_norflash_write(uint32_t addr, const uint8_t* p_data, uint32_t data_len);
// @brief 擦除某一块数据
int32_t htd_dev_norflash_erase(uint32_t addr, uint32_t size);


// @brief 按页读数据(page_idx 为页序号)
// @node 这里的页是一个擦除单位的大小
uint32_t htd_dev_norflash_page_read(uint32_t page_idx, uint8_t* p_data, uint32_t data_len);
// @brief 按页写数据, 该接口先擦后写，外部不用调用擦除函数(page_idx 为页序号)
// @node 这里的页是一个擦除单位的大小
uint32_t htd_dev_norflash_page_write(uint32_t page_idx, const uint8_t* p_data, uint32_t data_len);


// @brief 获取起始地址, 失败返回0xffffffff
uint32_t htd_dev_norflash_phy_saddr_get(void);
// @brief 获取结束地址, 失败返回0xffffffff
uint32_t htd_dev_norflash_phy_eaddr_get(void);
// @brief 获取大小
uint32_t htd_dev_norflash_size_get(void);
// @brief 获取擦除的大小，单位字节;
uint32_t htd_dev_norflash_erase_unit_get(void);
// @brief 获取页大小
uint32_t htd_dev_norflash_page_size_get(void);
// @brief 初始化
int32_t htd_dev_norflash_init(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_NORFLASH_H_*/

