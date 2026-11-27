// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG       "program.simulator"
#include "htd_elfdl_dev.h"
#include "htd_elfdl_process.h"


#if defined(ELFDL_LOAD_IN_SIMULATE)
#include "htd_vfs_process.h"
//////////////////////////////////////////////////////////////////////
typedef struct {
    htd_list_node_t node;
    char filepath[ELFDL_PATH_MAX_LEN];
    uint8_t* p_phy_addr;
    uint32_t size;
    uint32_t offset;
    uint8_t update;
}elfdl_dev_simu_t;
static htd_list_head_t m_program_list;

// @brief 信息打印
static void program_space_info_show(void)
{
    htd_list_node_t* p_pos;
    elfdl_dev_simu_t* p_dev;
    htd_list_for_each(p_pos, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_simu_t, node);
        print_debug("size=%d, offset=%d, p_phy_addr=%p, filepath=%s", p_dev->size, p_dev->offset, p_dev->p_phy_addr, p_dev->filepath);
    }
}

// @brief 创建程序节点
static elfdl_dev_simu_t* elfdl_dev_simu_space_node_create(const char* p_filepath, uint32_t filesize)
{
    if (filesize == 0 || p_filepath == NULL || p_filepath[0] == 0) {
        return NULL;
    }
    elfdl_dev_simu_t* p_dev = (elfdl_dev_simu_t*)htd_malloc(sizeof(elfdl_dev_simu_t));
    ASSERT(p_dev);
    htd_memset(p_dev, 0, sizeof(elfdl_dev_simu_t));
    htd_list_head_init(&p_dev->node);
    htd_strncpy(p_dev->filepath, p_filepath, sizeof(p_dev->filepath));
    p_dev->size = filesize;
    // 创建程序空间
    p_dev->p_phy_addr = htd_malloc(filesize);
    ASSERT(p_dev->p_phy_addr);
    htd_memset(p_dev->p_phy_addr, 0xff, filesize);
    return p_dev;
}

// @brief 销毁程序节点
static void elfdl_dev_simu_space_node_destroy(elfdl_dev_simu_t* p_dev)
{
    if (p_dev) {
        if (p_dev->p_phy_addr) {
            htd_free(p_dev->p_phy_addr);
        }
        htd_free(p_dev);
        unlink(p_dev->filepath);
    }
}

// @brief 创建一个程序空间
static elfdl_fd_t elfdl_dev_simu_create(const char* p_filepath, uint32_t filesize)
{
    elfdl_dev_simu_t* p_dev;
    htd_list_node_t* p_pos, *p_temp;
    uint32_t filepath_len = htd_strlen(p_filepath) + 1;
    htd_list_for_each_safe(p_pos, p_temp, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_simu_t, node);
        if (htd_memcmp(p_dev->filepath, p_filepath, filepath_len)) {
            continue;
        }
        htd_list_del(p_pos);
        elfdl_dev_simu_space_node_destroy(p_dev);
    }
    
    p_dev = elfdl_dev_simu_space_node_create(p_filepath, filesize);
    if (p_dev) {
        htd_list_insert_after(&p_dev->node, &m_program_list);
        return p_dev;
    }
    return NULL;
}

// @brief 删除
static int32_t elfdl_dev_simu_destroy(const char* p_filepath)
{
    elfdl_dev_simu_t* p_dev;
    htd_list_node_t* p_pos, *p_temp;
    uint32_t filepath_len = htd_strlen(p_filepath) + 1;
    htd_list_for_each_safe(p_pos, p_temp, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_simu_t, node);
        if (htd_memcmp(p_dev->filepath, p_filepath, filepath_len)) {
            continue;
        }
        htd_list_del(p_pos);
        elfdl_dev_simu_space_node_destroy(p_dev);
        return 0;
    }
    return -1;
}

// @brief 打开程序空间，已经存在了
static elfdl_fd_t elfdl_dev_simu_open(const char* p_filepath)
{
    elfdl_dev_simu_t* p_dev;
    htd_list_node_t* p_pos, *p_temp;
    uint32_t filepath_len = htd_strlen(p_filepath) + 1;
    htd_list_for_each_safe(p_pos, p_temp, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_simu_t, node);
        if (htd_memcmp(p_dev->filepath, p_filepath, filepath_len)) {
            continue;
        }
        p_dev->offset = 0;
        return p_dev;
    }
    return NULL;
}

// @brief 将一块数据保存为一个新的文件
static int32_t file_make(const char* p_filepath, const uint8_t* p_data, uint32_t data_len)
{
    int fd = open(p_filepath, O_BINARY|O_RDWR|O_CREAT);
    if (fd < 0) {
        print_error("create file failed. %s", p_filepath);
        return -1;
    }
    int32_t ret = write(fd, p_data, data_len);
    close(fd);
    return ret;
}

// @brief 关闭一个程序空间
static int32_t elfdl_dev_simu_close(elfdl_fd_t p_efd)
{
    // 将内存数据导入到文件中
    elfdl_dev_simu_t* p_dev = (elfdl_dev_simu_t*)p_efd;
    if (p_dev->update) {
        file_make(p_dev->filepath, p_dev->p_phy_addr, p_dev->size);
    }
    return 0;
}

// @brief 获取程序空间大小
static uint32_t elfdl_dev_simu_space_size_get(elfdl_fd_t p_efd)
{
    elfdl_dev_simu_t* p_dev = (elfdl_dev_simu_t*)p_efd;
    return p_dev->size;
}

// @brief 获取程序空间物理地址
static elf_addr_t elfdl_dev_simu_phy_addr_get(elfdl_fd_t p_efd)
{
    elfdl_dev_simu_t* p_dev = (elfdl_dev_simu_t*)p_efd;
    return (elf_addr_t)p_dev->p_phy_addr;
}

// @brief 写入数据
static int32_t elfdl_dev_simu_write(elfdl_fd_t p_efd, const void* p_data, uint32_t data_len)
{
    elfdl_dev_simu_t* p_dev = (elfdl_dev_simu_t*)p_efd;
    if (p_data == NULL || data_len == 0) {
        return 0;
    }
    if (p_dev->offset >= p_dev->size) {
        return 0;
    }
    p_dev->update = 1;
    if (p_dev->offset + data_len > p_dev->size) {
        data_len = p_dev->size - p_dev->offset;
    } 
    htd_memcpy(&p_dev->p_phy_addr[p_dev->offset], p_data, data_len);
    p_dev->offset += data_len;
    return data_len;
}

// @brief 程序是否存在
static int32_t elfdl_dev_simu_stat(const char* p_filepath)
{
    elfdl_dev_simu_t* p_dev;
    htd_list_node_t* p_pos, *p_temp;
    uint32_t filepath_len = htd_strlen(p_filepath);
    htd_list_for_each_safe(p_pos, p_temp, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_simu_t, node);
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

// @brief 目录删除
static int32_t elfdl_dev_simu_dir_delete(const char* p_dir_path)
{
    uint8_t ch;
    elfdl_dev_simu_t* p_dev;
    htd_list_node_t* p_pos, *p_temp;
    uint32_t filepath_len = htd_strlen(p_dir_path);
    htd_list_for_each_safe(p_pos, p_temp, &m_program_list) {
        p_dev = htd_list_entry(p_pos, elfdl_dev_simu_t, node);
        if (filepath_len != htd_strlen(p_dev->filepath)) {
            continue;
        }
        if (htd_memcmp(p_dev->filepath, p_dir_path, filepath_len)) {
            continue;
        }
        ch = p_dev->filepath[filepath_len];
        if (ch != '/' && ch != '\\') {
            continue;
        }
        htd_list_del(p_pos);
        elfdl_dev_simu_space_node_destroy(p_dev);
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t elfdl_dev_simu_register(void)
{
    htd_list_head_init(&m_program_list);
    const static elfdl_dev_t program_dev = {
        .malloc_max_size_kb = 65535,
        .is_ram = 1,        // 1
        .create_func = elfdl_dev_simu_create,
        .open_func = elfdl_dev_simu_open,
        .close_func = elfdl_dev_simu_close,
        .space_size_get_func = elfdl_dev_simu_space_size_get,
        .phy_addr_get_func = elfdl_dev_simu_phy_addr_get,
        .write_func = elfdl_dev_simu_write,
        .delete_func = elfdl_dev_simu_destroy,
        .delete_dir_func = elfdl_dev_simu_dir_delete,
        .stat_func = elfdl_dev_simu_stat,
    };
    return elfdl_dev_register(&program_dev);
}
DEVICE_INIT_FUNC_REGISTER(elfdl_dev_simu_register);


//////////////////////////////////////////////////////////////////////
#if (1 && defined(DEBUG_LVL) && (DEBUG_LVL >= 4) && defined(HTD_CONSOLE_ENABLE))
#include "htd_console_process.h"
// @brief 
int program_cmd_handle(int argc, void** argv)
{
    if (htd_strcmp(argv[1], "show") == 0) {
        program_space_info_show();
        return 0;
    } else {
        print_warning("Usage: %s show", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(program_cmd_handle, program);
#endif
#endif 

