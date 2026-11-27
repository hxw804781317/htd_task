// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_ELFDL_DEV_H_
#define _HTD_ELFDL_DEV_H_

#include "htd_elfdl_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
typedef void* elfdl_fd_t;

// 程序设备操作接口；程序设备必须支持内存映射功能(XIP)
typedef struct {
    uint16_t malloc_max_size_kb;            // 单次申请内存最大大小，单位KByte
    uint8_t  is_ram;                        // 是否为内存
    
    elfdl_fd_t (*create_func)(const char* p_filepath, uint32_t filesize);
    elfdl_fd_t (*open_func)(const char* p_filepath);
    int32_t (*close_func)(elfdl_fd_t p_fd);
    uint32_t (*space_size_get_func)(elfdl_fd_t p_fd);
    elf_addr_t (*phy_addr_get_func)(elfdl_fd_t p_fd);
    int32_t (*write_func)(elfdl_fd_t p_fd, const void* p_data, uint32_t data_len);
    int32_t (*delete_func)(const char* p_filepath);
    int32_t (*delete_dir_func)(const char* p_dir_path);
    int32_t (*stat_func)(const char* p_filepath);
} elfdl_dev_t;


// @brief 注册程序空间
int32_t elfdl_dev_register(const elfdl_dev_t* p_dev);
// @brief 程序空间是否为ram
bool elfdl_space_is_ram(void);
// @brief 创建一块程序空间
elfdl_fd_t elfdl_space_create(const char* p_filepath, uint32_t filesize);
// @brief 销毁程序空间(程序空间不在)
int32_t elfdl_space_destroy(const char* p_filepath);
// @brief 打开程序空间
elfdl_fd_t elfdl_space_open(const char* p_filepath);
// @brief 关闭程序空间(程序空间依旧在)
int32_t elfdl_space_close(elfdl_fd_t p_fd);
// @brief 程序空间大小
uint32_t elfdl_space_size_get(elfdl_fd_t p_fd);
// @brief 获取程序空间物理地址
elf_addr_t elfdl_space_addr_get(elfdl_fd_t p_fd);
// @brief 写程序空间
int32_t elfdl_space_write(elfdl_fd_t p_fd, const void* p_data, uint32_t data_len);
// @brief 删除程序空间目录
int32_t elfdl_space_dir_delete(const char* p_path);
// @brief 判断程序空间是否存在
int32_t elfdl_space_stat(const char* p_filepath);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_ELFDL_DEV_H_*/

