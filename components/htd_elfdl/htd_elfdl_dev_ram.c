// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG       "program.ram"
#include "htd_elfdl_dev.h"
#include "htd_elfdl_process.h"


#ifdef ELFDL_LOAD_IN_RAM
//////////////////////////////////////////////////////////////////////
typedef struct {
    htd_list_node_t node;
    char filepath[ELFDL_PATH_MAX_LEN];
    uint8_t* p_phy_addr;
    uint32_t size;
    uint32_t offset;
}elfdl_dev_ram_t;
static htd_list_head_t m_program_list;

// @brief 信息打印
static void elfdl_dev_ram_info_show(void)
{
    htd_list_node_t* p_pos;
    elfdl_dev_ram_t* p_dev;
    htd_list_for_each(p_pos, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_ram_t, node);
        print_debug("size=%d, offset=%d, p_phy_addr=%p, filepath=%s", p_dev->size, p_dev->offset, p_dev->p_phy_addr, p_dev->filepath);
    }
}

// @brief 创建程序节点
static elfdl_fd_t* elfdl_dev_ram_space_node_create(const char* p_filepath, uint32_t filesize)
{
    if (filesize == 0 || p_filepath == NULL || p_filepath[0] == 0) {
        return NULL;
    }
    elfdl_dev_ram_t* p_dev = (elfdl_dev_ram_t*)htd_malloc(sizeof(elfdl_dev_ram_t));
    ASSERT(p_dev);
    htd_memset(p_dev, 0, sizeof(elfdl_dev_ram_t));
    htd_list_head_init(&p_dev->node);
    htd_strncpy(p_dev->filepath, p_filepath, sizeof(p_dev->filepath));
    p_dev->size = filesize;
    // 申请对应的数据空间
    p_dev->p_phy_addr = htd_malloc(filesize);
    ASSERT(p_dev->p_phy_addr);
    htd_memset(p_dev->p_phy_addr, 0xff, filesize);
    return p_dev;
}
// @brief 销毁程序节点
static void elfdl_dev_ram_space_node_destroy(elfdl_fd_t* p_efd)
{
    elfdl_dev_ram_t* p_dev = (elfdl_dev_ram_t*)p_efd;
    if (p_dev) {
        if (p_dev->p_phy_addr) {
            htd_free(p_dev->p_phy_addr);
            p_dev->p_phy_addr = NULL;
        }
        htd_free(p_dev);
    }
}

// @brief 创建一个程序空间
static elfdl_fd_t elfdl_dev_ram_create(const char* p_filepath, uint32_t filesize)
{
    // 先遍历是否存在，存在则先删掉
    elfdl_dev_ram_t* p_dev;
    htd_list_node_t* p_pos, *p_temp;
    uint32_t filepath_len = htd_strlen(p_filepath) + 1;
    htd_list_for_each_safe(p_pos, p_temp, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_ram_t, node);
        if (htd_memcmp(p_dev->filepath, p_filepath, filepath_len)) {
            continue;
        }
        htd_list_del(p_pos);
        elfdl_dev_ram_space_node_destroy(p_dev);
    }

    // 创建
    p_dev = elfdl_dev_ram_space_node_create(p_filepath, filesize);
    if (p_dev) {
        htd_list_insert_after(&p_dev->node, &m_program_list);
        return (elfdl_fd_t)p_dev;
    }
    return NULL;
}

// @brief 打开程序空间，已经存在了
static elfdl_fd_t elfdl_dev_ram_open(const char* p_filepath)
{
    elfdl_dev_ram_t* p_dev;
    htd_list_node_t* p_pos, *p_temp;
    uint32_t filepath_len = htd_strlen(p_filepath) + 1;
    htd_list_for_each_safe(p_pos, p_temp, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_ram_t, node);
        if (htd_memcmp(p_dev->filepath, p_filepath, filepath_len)) {
            continue;
        }
        p_dev->offset = 0;
        return p_dev;
    }
    return NULL;
}

// @brief 关闭一个程序空间
static int32_t elfdl_dev_ram_close(elfdl_fd_t p_efd)
{
    return 0;
}

// @brief 获取程序空间大小
static uint32_t elfdl_dev_ram_space_size_get(elfdl_fd_t p_efd)
{
    elfdl_dev_ram_t* p_dev = (elfdl_dev_ram_t*)p_efd;
    return p_dev->size;
}

// @brief 获取程序空间物理地址
static elf_addr_t elfdl_dev_ram_phy_addr_get(elfdl_fd_t p_efd)
{
    elfdl_dev_ram_t* p_dev = (elfdl_dev_ram_t*)p_efd;
    return (elf_addr_t)p_dev->p_phy_addr;
}

// @brief 写入数据
static int32_t elfdl_dev_ram_write(elfdl_fd_t p_efd, const void* p_data, uint32_t data_len)
{
    elfdl_dev_ram_t* p_dev = (elfdl_fd_t*)p_efd;
    if (p_data == NULL || data_len == 0) {
        return 0;
    }
    if (p_dev->offset >= p_dev->size) {
        return 0;
    }
    if (p_dev->offset + data_len > p_dev->size) {
        data_len = p_dev->size - p_dev->offset;
    } 
    htd_memcpy(&p_dev->p_phy_addr[p_dev->offset], p_data, data_len);
    p_dev->offset += data_len;
    return data_len;
}

// @brief 程序是否存在
static int32_t elfdl_dev_ram_stat(const char* p_filepath)
{
    elfdl_dev_ram_t* p_dev;
    htd_list_node_t* p_pos, *p_temp;
    uint32_t filepath_len = htd_strlen(p_filepath);
    htd_list_for_each_safe(p_pos, p_temp, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_ram_t, node);
        if (filepath_len != htd_strlen(p_dev->filepath)) {
            continue;
        }
        if (htd_memcmp(p_dev->filepath, p_filepath, filepath_len)) {
            continue;
        }
        return 0;
    }
    return -1;
}

// @brief 删除
static int32_t elfdl_dev_ram_delete(const char* p_filepath)
{
    elfdl_dev_ram_t* p_dev;
    htd_list_node_t* p_pos, *p_temp;
    uint32_t filepath_len = htd_strlen(p_filepath) + 1;
    htd_list_for_each_safe(p_pos, p_temp, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_ram_t, node);
        if (htd_memcmp(p_dev->filepath, p_filepath, filepath_len)) {
            continue;
        }
        htd_list_del(p_pos);
        elfdl_dev_ram_space_node_destroy(p_dev);
        return 0;
    }
    return -1;
}

// @brief 目录删除
static int32_t elfdl_dev_ram_dir_delete(const char* p_dir_path)
{
    uint8_t ch;
    elfdl_dev_ram_t* p_dev;
    htd_list_node_t* p_pos, *p_temp;
    uint32_t filepath_len = htd_strlen(p_dir_path);
    htd_list_for_each_safe(p_pos, p_temp, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_ram_t, node);
        if (htd_memcmp(p_dev->filepath, p_dir_path, filepath_len)) {
            continue;
        }
        ch = p_dev->filepath[filepath_len];
        if (ch != '/' && ch != '\\') {
            continue;
        }
        htd_list_del(p_pos);
        elfdl_dev_ram_space_node_destroy(p_dev);
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t elfdl_dev_ram_register(void)
{
    htd_list_head_init(&m_program_list);
    const static elfdl_dev_t program_dev = {
        .malloc_max_size_kb = 65535,
        .is_ram = 1,
        .create_func = elfdl_dev_ram_create,
        .open_func = elfdl_dev_ram_open,
        .close_func = elfdl_dev_ram_close,
        .space_size_get_func = elfdl_dev_ram_space_size_get,
        .phy_addr_get_func = elfdl_dev_ram_phy_addr_get,
        .write_func = elfdl_dev_ram_write,
        .delete_func = elfdl_dev_ram_delete,
        .delete_dir_func = elfdl_dev_ram_dir_delete,
        .stat_func = elfdl_dev_ram_stat,
    };
    return elfdl_dev_register(&program_dev);
}
DEVICE_INIT_FUNC_REGISTER(elfdl_dev_ram_register);
#endif 


