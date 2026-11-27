// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG       "program.xip"
#include "htd_elfdl_dev.h"
#include "htd_elfdl_process.h"


#ifdef ELFDL_LOAD_IN_XIP
//////////////////////////////////////////////////////////////////////
// @brief 创建一个程序空间
static elfdl_fd_t elfdl_dev_xip_create(const char* p_filepath, uint32_t filesize)
{
    return NULL;
}

// @brief 打开程序空间，已经存在了
static elfdl_fd_t elfdl_dev_xip_open(const char* p_filepath)
{
    return NULL;
}

// @brief 关闭一个程序空间
static int32_t elfdl_dev_xip_close(elfdl_fd_t p_fd)
{
    return 0;
}

// @brief 获取程序空间大小
static uint32_t elfdl_dev_xip_space_size_get(elfdl_fd_t p_fd)
{
    return ;
}

// @brief 获取程序空间物理地址
static elf_addr_t elfdl_dev_xip_phy_addr_get(elfdl_fd_t p_fd)
{
    return ;
}

// @brief 写入数据
static int32_t elfdl_dev_xip_write(elfdl_fd_t p_fd, const void* p_data, uint32_t data_len)
{
    return data_len;
}

// @brief 程序是否存在
static int32_t elfdl_dev_xip_stat(const char* p_filepath)
{
    return -1;
}

// @brief 删除
static int32_t elfdl_dev_xip_delete(const char* p_filepath)
{
    return -1;
}

// @brief 目录删除
static int32_t elfdl_dev_xip_dir_delete(const char* p_dir_path)
{

}

// @brief 注册到上层应用中
static int32_t elfdl_dev_xip_register(void)
{
    const static elfdl_dev_t elfdl_dev_dev = {
        .malloc_max_size_kb = 32,
        .is_ram = 1,
        .create_func = elfdl_dev_xip_create,
        .open_func = elfdl_dev_xip_open,
        .close_func = elfdl_dev_xip_close,
        .space_size_get_func = elfdl_dev_xip_space_size_get,
        .phy_addr_get_func = elfdl_dev_xip_phy_addr_get,
        .write_func = elfdl_dev_xip_write,
        .delete_func = elfdl_dev_xip_delete,
        .delete_dir_func = elfdl_dev_xip_dir_delete,
        .stat_func = elfdl_dev_xip_stat,
    };
    return elfdl_dev_register(&elfdl_dev_dev);
}
DEVICE_INIT_FUNC_REGISTER(elfdl_dev_xip_register);
#endif 



