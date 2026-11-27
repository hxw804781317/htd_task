// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG       "program.dev"
#include "htd_elfdl_dev.h"


static const elfdl_dev_t* m_elfdl_dev = NULL;

// @brief 注册程序空间
int32_t elfdl_dev_register(const elfdl_dev_t* p_dev)
{
    ASSERT(p_dev);
    ASSERT(p_dev->create_func);
    ASSERT(p_dev->open_func);
    ASSERT(p_dev->close_func);
    ASSERT(p_dev->write_func);
    ASSERT(p_dev->delete_func);
    ASSERT(p_dev->stat_func);
    ASSERT(p_dev->space_size_get_func);
    ASSERT(p_dev->phy_addr_get_func);
    m_elfdl_dev = p_dev;
    return 0;
}

// @brief 程序空间是否为ram
bool elfdl_space_is_ram(void)
{
    ASSERT(m_elfdl_dev);
    return m_elfdl_dev->is_ram;
}

// @brief 创建一块程序空间
elfdl_fd_t elfdl_space_create(const char* p_filepath, uint32_t filesize)
{
    ASSERT(m_elfdl_dev);
    return m_elfdl_dev->create_func(p_filepath, filesize);
}

// @brief 销毁程序空间(程序空间不在)
int32_t elfdl_space_destroy(const char* p_filepath)
{
    ASSERT(m_elfdl_dev);
    return m_elfdl_dev->delete_func(p_filepath);
}

// @brief 打开程序空间
elfdl_fd_t elfdl_space_open(const char* p_filepath)
{
    ASSERT(m_elfdl_dev);
    return m_elfdl_dev->open_func(p_filepath);
}

// @brief 关闭程序空间(程序空间依旧在)
int32_t elfdl_space_close(elfdl_fd_t p_fd)
{
    ASSERT(m_elfdl_dev);
    return m_elfdl_dev->close_func(p_fd);
}

// @brief 程序空间大小
uint32_t elfdl_space_size_get(elfdl_fd_t p_fd)
{
    ASSERT(m_elfdl_dev);
    return m_elfdl_dev->space_size_get_func(p_fd);
}

// @brief 获取程序空间物理地址
elf_addr_t elfdl_space_addr_get(elfdl_fd_t p_fd)
{
    ASSERT(m_elfdl_dev);
    return m_elfdl_dev->phy_addr_get_func(p_fd);
}

// @brief 写程序空间
int32_t elfdl_space_write(elfdl_fd_t p_fd, const void* p_data, uint32_t data_len)
{
    ASSERT(m_elfdl_dev);
    return m_elfdl_dev->write_func(p_fd, p_data, data_len);
}

// @brief 删除程序空间目录
int32_t elfdl_space_dir_delete(const char* p_path)
{
    ASSERT(m_elfdl_dev);
    return m_elfdl_dev->delete_dir_func(p_path);
}

// @brief 判断程序空间是否存在
int32_t elfdl_space_stat(const char* p_filepath)
{
    ASSERT(m_elfdl_dev);
    return m_elfdl_dev->stat_func(p_filepath);
}


