// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG              "vfs"
#include "htd_vfs_port.h"
#include "htd_vfs_process.h"
#include "htd_vfs_file.h"


//////////////////////////////////////////////////////////////////////
HTD_SECTION_DEF(HTD_VFS_API_SECTION_NAME, htd_vfs_api_t);

typedef struct {
    htd_slist_node_t slist;
    uint16_t count;
    uint16_t is_init;
    char* p_work_dir;
} vfs_filesystem_table_t;
static vfs_filesystem_table_t m_filesystem = {.is_init = 0};
static vfs_mutex_lock_t *m_vfs_lock = NULL;


// @brief 判断是否已经初始化
static bool htd_vfs_is_init(void)
{
    return (m_filesystem.is_init == 0xa5);
}

// @brief 加锁
void htd_vfs_lock(void)
{
    htd_mutex_lock(m_vfs_lock);
}

// @brief 解锁
void htd_vfs_unlock(void)
{
    htd_mutex_unlock(m_vfs_lock);
}

// @brief 初始化
int htd_vfs_init(void)
{
    if (m_filesystem.is_init == 0xa5) {
        print_error("fs already init.");
        return 0;
    }

    m_filesystem.count = 0;
    m_filesystem.slist.next = NULL;
    m_vfs_lock = vfs_mutex_lock_create("vfs-lock");
    ASSERT(m_vfs_lock);
    m_filesystem.p_work_dir = htd_malloc(2);
    ASSERT(m_filesystem.p_work_dir);
    m_filesystem.p_work_dir[0] = '/';
    m_filesystem.p_work_dir[1] = 0;
    m_filesystem.is_init = 0xa5;
    return 0;
}

// @brief 获取工作路径
const char* htd_vfs_work_dir_get(void)
{
    return m_filesystem.p_work_dir;
}

// @brief 设置工作路径
void htd_vfs_work_dir_set(const char* p_path)
{
    ASSERT(htd_vfs_is_init());
    uint32_t path_len = htd_strlen(p_path);
    htd_vfs_lock();
    if (path_len <= htd_strlen(m_filesystem.p_work_dir)) {
        htd_memcpy(m_filesystem.p_work_dir, p_path, path_len);
        m_filesystem.p_work_dir[path_len] = 0;
    } else {
        path_len += 1;
        htd_free(m_filesystem.p_work_dir);
        m_filesystem.p_work_dir = (char*)htd_malloc(path_len);
        ASSERT(m_filesystem.p_work_dir);
        htd_memcpy(m_filesystem.p_work_dir, p_path, path_len);
    }
    htd_vfs_unlock();
}

// @brief 查找对应的设备
static const htd_vfs_api_t* vfs_fs_api_find(htd_vfs_type_t fs_type)
{
    htd_vfs_api_t* p_api;
    const uint32_t* start = (uint32_t*)HTD_SECTION_START_ADDR(HTD_VFS_API_SECTION_NAME);
    const uint32_t* end = (uint32_t*)HTD_SECTION_END_ADDR(HTD_VFS_API_SECTION_NAME);
    const uint32_t* p_item = htd_section_first_item_get(start, end);
    while (p_item) {
        p_api = (htd_vfs_api_t*)p_item;
        if (p_api->magic == HTD_VFS_MAGIC && p_api->type == fs_type) {
            return p_api;
        }
        p_item = htd_section_next_item_get(p_item, end, sizeof(htd_vfs_api_t));
    }
    return NULL;
}

// @brief 通过挂载的文件路径进行查找
htd_vfs_t *htd_vfs_find(const char *p_path)
{
    ASSERT(p_path);
    ASSERT(htd_vfs_is_init());

    htd_vfs_t* p_find = NULL;
    htd_vfs_t* p_iter;
    htd_slist_node_t* p_pos;
    uint32_t prefix_len = 0;
    uint32_t fs_path_len;
    uint32_t path_len = htd_strlen(p_path);

    htd_vfs_lock();
    htd_slist_for_each(p_pos, &m_filesystem.slist) {
        p_iter = htd_slist_entry(p_pos, htd_vfs_t, node);
        ASSERT(p_iter->p_path);
        fs_path_len = htd_strlen(p_iter->p_path);
        if (fs_path_len < prefix_len || htd_memcmp(p_path, p_iter->p_path, fs_path_len)) {
            continue;
        }
        if (fs_path_len > 1 && path_len > fs_path_len && (p_path[fs_path_len] != '/')) {
            continue;
        }
        p_find = p_iter;
        prefix_len = fs_path_len;
    }
    htd_vfs_unlock();
    return p_find;
}

// @brief 初始化vfs值
static bool htd_vfs_val_init(htd_vfs_t *p_vfs, htd_vfs_type_t fs_type, const char *p_fullpath)
{
    if (fs_type == HTD_VFS_TYPE_NONE) {
        htd_vfs_t *p_find = htd_vfs_find(p_fullpath);
        if (p_find == NULL) {
            return false;
        }
        htd_memcpy(p_vfs, p_find, sizeof(htd_vfs_t));
    } else {
        const htd_vfs_api_t* p_api = vfs_fs_api_find(fs_type);
        if (p_api == NULL) {
            return false;
        }
        p_vfs->type = p_api->type;
        p_vfs->f_open = p_api->f_open;
        p_vfs->f_close = p_api->f_close;
        p_vfs->f_ioctl = p_api->f_ioctl;
        p_vfs->f_read = p_api->f_read;
        p_vfs->f_write = p_api->f_write;
        p_vfs->f_lseek = p_api->f_lseek;
        p_vfs->f_flush = p_api->f_flush;
        p_vfs->f_getdents = p_api->f_getdents;
        p_vfs->f_unlink = p_api->f_unlink;
        p_vfs->f_stat = p_api->f_stat;
        p_vfs->f_rename = p_api->f_rename;
        p_vfs->fs_mount = p_api->fs_mount;
        p_vfs->fs_unmount = p_api->fs_unmount;
        p_vfs->fs_mk = p_api->fs_mk;
        p_vfs->fs_stat = p_api->fs_stat;
    }
    return true;
}

// @brief 挂载文件系统
// @param fs_type 文件系统类型
// @param p_path 挂载的路径
// @param data 用户数据
// @param is_force true挂载失败时会直接格式化，再重新挂载
// @return 成功返回0，失败返回非0数值
int htd_vfs_mount(htd_vfs_type_t fs_type, const char *p_path, const void *fs_dev, bool is_force)
{
    ASSERT(htd_vfs_is_init());
    if (m_filesystem.count >= VFS_FILESYSTEMS_MAX) {
        return (-ERROR_CODE_NO_SPACE);
    }
    
    char* p_fullpath = htd_vfs_path_linux_standard(NULL, p_path);
    if (p_fullpath == NULL)  {
        return (-ERROR_CODE_NO_SPACE);
    }
    if ((htd_strcmp(p_fullpath, "/") != 0) && (htd_strcmp(p_fullpath, "/dev") != 0)) {
        int fd = htd_vfs_file_open(p_fullpath, O_DIRECTORY|O_CREAT);
        if (fd < 0) {
            htd_free(p_fullpath);
            return (-ERROR_CODE_FAILED);
        }
        htd_vfs_file_close(fd);
    }

    int32_t ret = 0;
    htd_vfs_t *p_vfs = (htd_vfs_t*)htd_malloc(sizeof(htd_vfs_t));
    ASSERT(p_vfs);
    htd_memset(p_vfs, 0, sizeof(htd_vfs_t));
    
    htd_vfs_lock();
    do {
        if (htd_vfs_val_init(p_vfs, fs_type, p_fullpath) == false) {
            break;
        }
        p_vfs->p_fs_dev = (void*)fs_dev;
        p_vfs->p_path = p_fullpath;
        
        // 挂载
        ret = p_vfs->fs_mount(p_vfs, fs_dev);
        if (ret == ERROR_CODE_SUCCESS) {
            break;
        } 
        if (is_force == false || p_vfs->fs_mk == NULL) {
            break;
        }
        print_debug("mkfs");
        ret = p_vfs->fs_mk(p_vfs, fs_dev);
        if (ret != ERROR_CODE_SUCCESS) {
            print_error("mkfs failed. ret=%d", ret);
            break;
        }
        ret = p_vfs->fs_mount(p_vfs, fs_dev);
        if (ret != ERROR_CODE_SUCCESS) {
            print_error("mount again failed. ret=%d", ret);
            break;
        }
    }while (0);
    
    // 加到链表
    if (ret == ERROR_CODE_SUCCESS) {
        htd_slist_append(&m_filesystem.slist, &p_vfs->node);
        m_filesystem.count++;
    } else {
        htd_free(p_fullpath);
        htd_free(p_vfs);
    }
    htd_vfs_unlock();
    return ret;
}

// @brief 通过路径卸载文件系统
// @param p_path 挂载的路径
// @return 成功返回0，失败返回小于0
int htd_vfs_unmount(const char *p_path)
{
    ASSERT(htd_vfs_is_init());
    char *p_fullpath = htd_vfs_path_linux_standard(NULL, p_path);
    if (p_fullpath == NULL) {
        return -ERROR_CODE_NO_SPACE;
    }
    uint32_t fullpath_len = htd_strlen(p_fullpath) + 1;
    
    htd_vfs_lock();
    htd_vfs_t *p_iter;
    htd_slist_node_t *p_pos;
    htd_slist_for_each(p_pos, &m_filesystem.slist) {
        p_iter = htd_slist_entry(p_pos, htd_vfs_t, node);
        if (p_iter->p_path != NULL && htd_memcmp(p_iter->p_path, p_fullpath, fullpath_len) == 0) {
            htd_free(p_iter->p_path);
            p_iter->p_path = NULL;
            if (p_iter->fs_unmount) {
                p_iter->fs_unmount(p_iter);
            }
            break;
        }
    }
    htd_vfs_unlock();
    htd_free(p_fullpath);
    return 0;
}

// @brief 迭代查询文件系统
htd_vfs_t *htd_vfs_query(htd_vfs_t *p_vfs)
{
    ASSERT(htd_vfs_is_init());
    htd_vfs_t* p_find = NULL;
    htd_vfs_t* p_iter;
    htd_slist_node_t* p_pos;
    htd_vfs_lock();
    htd_slist_for_each(p_pos, &m_filesystem.slist) {
        p_iter = htd_slist_entry(p_pos, htd_vfs_t, node);
        if (p_iter->p_path == NULL) {
            continue;
        } 
        if (p_vfs == NULL) {
            p_find = p_iter;
            break;
        }
        if (p_iter == p_vfs) {
            p_vfs = NULL;
        }
    }
    htd_vfs_unlock();
    return p_find;
}

// @brief 创建文件系统(该文件系统必须已经挂载)
// @param p_fs_name 文件系统名字
// @return 成功返回0，失败返回负值
int htd_vfs_mkfs(const char *p_path, void* p_param)
{
    ASSERT(htd_vfs_is_init());
    char *p_fullpath = htd_vfs_path_linux_standard(NULL, p_path);
    if (p_fullpath == NULL) {
        return (-ERROR_CODE_NO_SPACE);
    }

    int32_t ret = -(ERROR_CODE_FAILED);
    htd_vfs_lock();
    htd_vfs_t *p_vfs = htd_vfs_find(p_fullpath);
    if (p_vfs && p_vfs->fs_mk) {
        ret = p_vfs->fs_mk(p_vfs, p_param);
    } 
    htd_vfs_unlock();
    htd_free(p_fullpath);
    return ret;
}

// @brief 获取文件系统信息
int htd_vfs_statfs(const char *p_path, htd_vfs_statfs_t *p_statfs)
{
    htd_vfs_t *p_vfs = htd_vfs_find(p_path);
    if (p_vfs != NULL) {
        if (p_vfs->fs_stat != NULL) {
            return p_vfs->fs_stat(p_vfs, p_statfs);
        }
    }
    return (-ERROR_CODE_NOT_FOUND);
}

// @brief 显示文件系统信息
int htd_vfs_df(const char *path)
{
    int result;
    int minor = 0;
    uint64_t cap;
    htd_vfs_statfs_t buffer;

    int unit_index = 0;
    char *unit_str[] = {"KB", "MB", "GB"};

    htd_memset(&buffer, 0, sizeof(htd_vfs_statfs_t));
    result = htd_vfs_statfs(path ? path : NULL, &buffer);
    if (result != 0) {
        print_error("fs stat failed.");
        return -ERROR_CODE_INVALID_STATE;
    }

    cap = (uint64_t)buffer.f_bsize * (uint64_t)buffer.f_bfree;
    cap = cap / 1024;
    for (unit_index=0; unit_index<2; unit_index++) {
        if (cap < 1024) {
            break;
        }
        minor = ((cap % 1024) * 10) / 1024; /* only one decimal point */
        cap = cap / 1024LL;
    }
    htd_print("disk free: %d.%d %s [ %lld block, %d bytes per block ]\n",
               (unsigned long)cap, minor, unit_str[unit_index], buffer.f_blocks, buffer.f_bsize);
    return 0;
}

// @brief 获取文件系统剩余空间大小
uint32_t htd_vfs_free_space_size_get(const char *p_path)
{
    htd_vfs_statfs_t buffer;
    int result = htd_vfs_statfs(p_path, &buffer);
    ASSERT(result == ERROR_CODE_SUCCESS);
    return buffer.f_bsize * buffer.f_bfree;
}

