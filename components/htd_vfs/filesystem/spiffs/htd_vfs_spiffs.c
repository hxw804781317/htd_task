// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL   4
#define DEBUG_TAG              "vfs.spiffs"
#include <htd_vfs_spiffs.h>

// 适合 norflash
#if defined(HTD_VFS_ENABLE) && defined(HTD_VFS_USING_SPIFFS)
//////////////////////////////////////////////////////////////////////
#include <htd_vfs_process.h>
#include "spiffs.h"
#include "spiffs_nucleus.h"

typedef struct {
    htd_mutex_lock_t *lock;
    uint8_t *cache, *work, *p_fd;
    uint32_t cache_size, work_size, fd_size;
} spiffs_user_data_t;


// @brief 加锁
void vfs_spiffs_lock(void* p_fs)
{
    spiffs *p_spifs = (spiffs *)p_fs;
    spiffs_user_data_t* p_user_data = (spiffs_user_data_t*)p_spifs->user_data;
    vfs_mutex_lock(p_user_data->lock);
}

// @brief 解锁
void vfs_spiffs_unlock(void* p_fs)
{
    spiffs *p_spifs = (spiffs *)p_fs;
    spiffs_user_data_t* p_user_data = (spiffs_user_data_t*)p_spifs->user_data;
    vfs_mutex_unlock(p_user_data->lock);
}

// @brief 初始化配置信息
static int32_t vfs_spiffs_config_init(spiffs_config* p_cfg, spiffs_dev_t* p_dev)
{
    if (p_dev == NULL) {
        return -1;
    }
    p_cfg->hal_read_f = p_dev->read_handle;
    p_cfg->hal_write_f = p_dev->write_handle;
    p_cfg->hal_erase_f = p_dev->erase_handle;
    p_cfg->phys_size = p_dev->block_count * p_dev->block_size;
    p_cfg->phys_addr = p_dev->phy_addr;
    p_cfg->phys_erase_block = p_dev->block_size;
    p_cfg->log_block_size = p_dev->block_size * 4;
    p_cfg->log_page_size = p_cfg->log_block_size / 32;
#if SPIFFS_FILEHDL_OFFSET
    p_cfg->fh_ix_offset = VFS_FD_OFFSET;
#endif
    return 0;
}

// @brief 创建
spiffs* vfs_spiffs_create(uint32_t log_page_size)
{
    uint32_t work_size = log_page_size * 2;
    uint32_t fd_size = HTD_VFS_FD_MAX * sizeof(spiffs_fd);
    uint32_t cache_size = (log_page_size + 32) * 4;
    spiffs_user_data_t* p_user_data;
    spiffs *p_spifs = (spiffs*)htd_malloc(sizeof(spiffs) + sizeof(spiffs_user_data_t) + fd_size + work_size + cache_size);
    if (p_spifs == NULL) {
        return NULL;
    }
    htd_memset(p_spifs, 0, sizeof(spiffs));
    p_user_data = (spiffs_user_data_t*)(p_spifs + 1);
    p_user_data->p_fd = (uint8_t*)(p_user_data + 1);
    p_user_data->work = p_user_data->p_fd + fd_size;
    p_user_data->cache = p_user_data->work + work_size;
    p_user_data->work_size = work_size;
    p_user_data->cache_size = cache_size;
    p_user_data->fd_size = fd_size;
    p_spifs->user_data = p_user_data;
    //p_spifs->config_magic = SPIFFS_CONFIG_MAGIC;
    p_user_data->lock = vfs_mutex_lock_create("spiffs");
    VFS_ASSERT(p_user_data->lock);
    return p_spifs;
}

// @brief 销毁
static void vfs_spiffs_destroy(spiffs* p_spifs)
{
    if (p_spifs) {
        spiffs_user_data_t* p_user_data = (spiffs_user_data_t*)p_spifs->user_data;
        if (p_user_data && p_user_data->lock) {
            vfs_mutex_lock_destroy(p_user_data->lock);
            p_user_data->lock = NULL;
        }
        htd_free(p_spifs);
    }
}

// @brief 挂载
static int vfs_spiffs_mount(htd_vfs_t *p_vfs, const void *data)
{
    spiffs_config fs_cfg;
    if (0 != vfs_spiffs_config_init(&fs_cfg, (spiffs_dev_t*)data)) {
        return -EIO;
    }
    
    // 创建
    spiffs *p_spifs = vfs_spiffs_create(fs_cfg.log_page_size);
    if (p_spifs == NULL) {
        return -ERROR_CODE_NO_MEM; 
    }
    htd_memcpy(&p_spifs->cfg, &fs_cfg, sizeof(spiffs_config));

    // 挂载
    spiffs_user_data_t* p_info = (spiffs_user_data_t*)p_spifs->user_data;
    int32_t ret = SPIFFS_mount(p_spifs, &fs_cfg, p_info->work, p_info->p_fd, p_info->fd_size, p_info->cache, p_info->cache_size, NULL);
    if (SPIFFS_OK != ret) {
        htd_print("spiffs mount failed. ret=%d\n", ret);
        SPIFFS_unmount(p_spifs);
        // 失败格式化
        ret = SPIFFS_format(p_spifs);
        if (ret != 0) {
            htd_print("spiffs format failed. ret=%d\n", ret);
            return ret;
        }
        // 重新挂载
        ret = SPIFFS_mount(p_spifs, &fs_cfg, p_info->work, p_info->p_fd, p_info->fd_size, p_info->cache, p_info->cache_size, NULL);
        if (SPIFFS_OK != ret) {
            SPIFFS_unmount(p_spifs);
            vfs_spiffs_destroy(p_spifs);
            return ret;
        }
    }
    p_vfs->p_fs_dev = p_spifs;
    return 0;
}

// @brief 卸载
static int vfs_spiffs_unmount(htd_vfs_t *p_vfs)
{
    spiffs *p_spifs = (spiffs *)p_vfs->p_fs_dev;
    VFS_ASSERT(p_spifs);
    // 卸载
    SPIFFS_unmount(p_spifs);
    vfs_spiffs_destroy(p_spifs);
    return 0;
}

// @brief 查看文件系统信息
static int vfs_spiffs_statfs(htd_vfs_t *p_vfs, htd_vfs_statfs_t *p_buff)
{
    spiffs *p_spifs = (spiffs *)p_vfs->p_fs_dev;
    VFS_ASSERT(p_spifs);

    uint32_t total = 0;
    uint32_t used = 0;
    int32_t ret = SPIFFS_info(p_spifs, &total, &used);
    if (ret != 0) {
        return ret;
    }
    
    // 填充信息
    p_buff->f_bfree = total - used;
    p_buff->f_blocks = total / p_spifs->cfg.log_block_size;
    p_buff->f_bsize = total;
    return 0;
}

// @brief IO 控制
static int vfs_spiffs_ioctl(htd_vfs_fd_t *p_fd, int cmd, void *args)
{
    return -ERROR_CODE_FAILED;
}

// @brief 读数据
static int vfs_spiffs_read(htd_vfs_fd_t *p_fd, void *p_buff, size_t buff_len)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_FLAGS; 
    }
    return SPIFFS_read((spiffs*)p_fd->p_vfs->p_fs_dev, p_fd->fd, p_buff, buff_len);
}

// @brief 写数据
static int vfs_spiffs_write(htd_vfs_fd_t *p_fd, const void *p_buff, size_t buff_len)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_FLAGS;
    }
    return SPIFFS_write((spiffs*)p_fd->p_vfs->p_fs_dev, p_fd->fd, (void*)p_buff, buff_len);
}

// @brief 刷新文件
static int vfs_spiffs_flush(htd_vfs_fd_t *p_fd)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_FLAGS;
    }
    return SPIFFS_fflush((spiffs*)p_fd->p_vfs->p_fs_dev, p_fd->fd);
}

// @brief 文件位置
static int vfs_spiffs_lseek(htd_vfs_fd_t *p_fd, size_t offset)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_FLAGS;
    }
    return SPIFFS_lseek((spiffs*)p_fd->p_vfs->p_fs_dev, p_fd->fd, offset, SPIFFS_SEEK_SET);
}

// @brief 关闭
static int vfs_spiffs_close(htd_vfs_fd_t *p_fd)
{
    if (p_fd->flags & O_DIRECTORY) {
        return SPIFFS_closedir((spiffs_DIR*)p_fd->data);
    } else {
        return SPIFFS_close((spiffs*)p_fd->p_vfs->p_fs_dev, p_fd->fd);
    }
}

// @brief 打开
static int vfs_spiffs_open(htd_vfs_fd_t *p_fd)
{
    if (p_fd->flags & O_DIRECTORY) {
        spiffs_DIR* p_dir = (spiffs_DIR*)htd_malloc(sizeof(spiffs_DIR));
        if (p_dir == NULL) {
            return -ERROR_CODE_NO_MEM;
        }
        SPIFFS_opendir((spiffs*)p_fd->p_vfs->p_fs_dev, p_fd->path, p_dir);
        p_fd->data = p_dir;
        return 0;
    } 

    spiffs_flags flags = 0;
    if (p_fd->flags & O_CREAT) {
        flags |= SPIFFS_O_CREAT;
    }
    if (p_fd->flags & O_APPEND) {
        flags |= SPIFFS_O_APPEND;
    }
    if (p_fd->flags & O_WRONLY) {
        flags |= SPIFFS_O_WRONLY;
    }
    if (p_fd->flags & O_RDWR) {
        flags |= SPIFFS_O_RDWR;
    }
    if (p_fd->flags & O_TRUNC) {
        flags |= SPIFFS_O_TRUNC;
    }
    flags |= SPIFFS_O_RDONLY;
    int32_t fd = SPIFFS_open((spiffs*)p_fd->p_vfs->p_fs_dev, p_fd->path, flags, 0);
    if (fd < 0) {
        return fd;
    }
    
    spiffs_stat stat_val;
    SPIFFS_fstat((spiffs*)p_fd->p_vfs->p_fs_dev, fd, &stat_val);
    p_fd->fd = fd;
    p_fd->size = stat_val.size;
    if (p_fd->flags & O_APPEND) {
        p_fd->pos = p_fd->size;
    } else {
        p_fd->pos = 0;
    }
    return 0;
}

// @brief 查看文件信息
static int vfs_spiffs_stat(htd_vfs_t *p_vfs, const char *p_path, htd_vfs_stat_t *p_info)
{
    spiffs_stat stat_val;
    int32_t ret = SPIFFS_stat((spiffs*)p_vfs->p_fs_dev, p_path, &stat_val);
    if (ret != 0) {
        return ret;
    }
    
    p_info->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
    if (stat_val.type == SPIFFS_TYPE_DIR) {
        p_info->st_mode &= ~S_IFREG;
        p_info->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
    } 
    p_info->st_size = stat_val.size;
    return 0;
}

// @brief 获取目录信息
static int vfs_spiffs_getdents(htd_vfs_fd_t *p_fd, htd_vfs_dirent_t *p_entry)
{
    if (p_fd->type != FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_FLAGS;
    }

    struct spiffs_dirent dirent_val;
    if (NULL == SPIFFS_readdir((spiffs_DIR*)p_fd->data, &dirent_val)) {
        return 0;
    }
    
    p_entry->d_type = dirent_val.type == SPIFFS_TYPE_DIR ? DT_DIR : DT_REG;
    p_entry->d_namlen = htd_strlen(dirent_val.name);
    p_entry->d_reclen = sizeof(htd_vfs_dirent_t);
    if (dirent_val.name[0] == '/') {
        htd_strncpy(p_entry->d_name, &dirent_val.name[1], sizeof(p_entry->d_name)-1);
    } else {
        htd_strncpy(p_entry->d_name, &dirent_val.name[0], sizeof(p_entry->d_name)-1);
    }
    return sizeof(htd_vfs_dirent_t);
}

// @brief 删除
static int vfs_spiffs_unlink(htd_vfs_t *p_vfs, const char *p_path)
{
    return SPIFFS_remove((spiffs*)p_vfs->p_fs_dev, p_path);
}

// @brief 重命名
static int vfs_spiffs_rename(htd_vfs_t *p_vfs, const char *oldpath, const char *newpath)
{
    return SPIFFS_rename((spiffs*)p_vfs->p_fs_dev, oldpath, newpath);
}
HTD_VFS_API_REGISTER(HTD_VFS_TYPE_SPIFFS, \
                       vfs_spiffs_open, vfs_spiffs_close, vfs_spiffs_ioctl, \
                       vfs_spiffs_read, vfs_spiffs_write, vfs_spiffs_lseek, vfs_spiffs_flush, \
                       vfs_spiffs_getdents, vfs_spiffs_unlink, vfs_spiffs_stat, \
                       vfs_spiffs_rename, vfs_spiffs_mount, vfs_spiffs_unmount, NULL, vfs_spiffs_statfs);
#endif /*defined(HTD_VFS_ENABLE) && defined(VFS_USING_SPIFFS)*/

