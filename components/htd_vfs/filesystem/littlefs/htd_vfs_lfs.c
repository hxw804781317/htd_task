// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include <htd_vfs_lfs.h>

// 适合 norflash
#if defined(HTD_VFS_ENABLE) && defined(HTD_VFS_USING_LITTLE_FS)
//////////////////////////////////////////////////////////////////////
#include <htd_vfs_process.h>
#include "lfs.h"

typedef struct {
    lfs_dev_t dev;
    vfs_mutex_lock_t *lock;
} lfs_user_data_t;

uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size) 
{
#if 1
    static const uint32_t rtable[16] = {
        0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
        0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
        0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
        0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
    };
    const uint8_t *data = buffer;
    for (size_t i = 0; i < size; i++) {
        crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 0)) & 0xf];
        crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 4)) & 0xf];
    }
    return crc;
#else
    htd_crc32_ctx_t ctx = m_crc32_ctx_gen_def;
    ctx.result = crc;
    htd_crc32_init(&ctx);
    htd_crc32_update(&ctx, (const uint8_t*)buffer, size);
    htd_crc32_final(&ctx);
    return ctx.result;
#endif
}

// @brief 读句柄
static int lfs_read_handle(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    lfs_user_data_t* p_user_data = (lfs_user_data_t*)c->context;
    uint32_t addr = p_user_data->dev.phy_addr + block * c->block_size + off;
    if (size != p_user_data->dev.read_handle(addr, buffer, size)) {
        return LFS_ERR_IO;
    }
    return 0;
}
// @brief 写句柄
static int lfs_write_handle(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    lfs_user_data_t* p_user_data = (lfs_user_data_t*)c->context;
    uint32_t addr = p_user_data->dev.phy_addr + block * c->block_size + off;
    if (size != p_user_data->dev.write_handle(addr, buffer, size)) {
        return LFS_ERR_IO;
    }
    return 0;
}
// @brief 擦除
static int lfs_erase_handle(const struct lfs_config *c, lfs_block_t block)
{
    lfs_user_data_t* p_user_data = (lfs_user_data_t*)c->context;
    uint32_t addr = p_user_data->dev.phy_addr + block * c->block_size;
    p_user_data->dev.erase_handle(addr, c->block_size);
    return 0;
}
// @brief 同步
static int lfs_sync_handle(const struct lfs_config *c)
{
    return 0;
}
// @brief 加锁
static int lfs_lock_handle(const struct lfs_config *c)
{
    lfs_user_data_t* p_user_data = (lfs_user_data_t*)c->context;
    vfs_mutex_lock(p_user_data->lock);
    return 0;
}
// @brief 解锁
static int lfs_unlock_handle(const struct lfs_config *c)
{
    lfs_user_data_t* p_user_data = (lfs_user_data_t*)c->context;
    vfs_mutex_unlock(p_user_data->lock);
    return 0;
}

// @brief 初始化配置信息
static void lfs_config_init(struct lfs_config *p_cfg, lfs_dev_t* p_dev, lfs_user_data_t* p_user_data)
{
    htd_memset(p_cfg, 0, sizeof(struct lfs_config));
    htd_memcpy(&p_user_data->dev, p_dev, sizeof(lfs_dev_t));
    p_cfg->context = p_user_data;
    p_cfg->read = lfs_read_handle;
    p_cfg->prog = lfs_write_handle;
    p_cfg->erase = lfs_erase_handle;
    p_cfg->sync = lfs_sync_handle;
    p_cfg->lock = lfs_lock_handle;
    p_cfg->unlock = lfs_unlock_handle;
    p_cfg->read_size = 32; 
    p_cfg->prog_size = p_cfg->read_size;
    p_cfg->block_size = p_dev->block_size;
    p_cfg->block_count = p_dev->block_count;
    // 文件系统进行垃圾回收时block的擦除次数，推荐值100-1000.值越大垃圾回收次数越少
    p_cfg->block_cycles = 100;
    /* lfs需要read cache、program cache每个文件也需要cache, cache越大性能越好，
       会减少flash的访问次数，chche是读写大小的倍数，是擦除大小的因数 */
    p_cfg->cache_size = p_cfg->prog_size * 4;
    if (p_cfg->cache_size < p_cfg->block_size / 32) {
        p_cfg->cache_size = p_cfg->block_size / 32;
    }
    // 前瞻缓冲区的大小，增加了分配块时找到块的数量
    p_cfg->lookahead_size = 32 * ((p_cfg->block_count + 31) / 32);
    if (p_cfg->lookahead_size > 128) {
        p_cfg->lookahead_size = 128;
    }
    // 用于压缩元数据的阈值
    //p_cfg->compact_thresh = -1;
    // 读缓冲区, 默认为 lfs_malloc 申请, 指定必须为 cache_size 的内存
    //p_cfg->read_buffer = NULL;
    // 写缓冲区，默认为 lfs_malloc 申请, 指定必须为 cache_size 的内存
    //p_cfg->prog_buffer = NULL;
    // 预读缓冲区, 默认为 lfs_malloc 申请, 指定必须为 lookahead_size 的内存 
    //p_cfg->lookahead_buffer = NULL;
    // 文件名的最大长度
    p_cfg->name_max = VFS_PATH_MAX; 
    // 文件的最大长度
    //p_cfg->file_max = ;
    // 用户属性的最大长度
    //p_cfg->attr_max = ;
    // 用于存储元数据的空间上限
    //p_cfg->metadata_max = ;
    // 内联文件大小上限
    //p_cfg->inline_max = ;
}

// @brief 挂载
int vfs_lfs_mount(htd_vfs_t *p_vfs, const void *data)
{
    lfs_dev_t* p_dev = (lfs_dev_t*)data;
    if (p_dev == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }

    lfs_t * p_lfs = htd_malloc(sizeof(lfs_t) + sizeof(struct lfs_config) + sizeof(lfs_user_data_t));
    if (p_lfs == NULL) {
        return -ERROR_CODE_NO_MEM;
    }
    htd_memset(p_lfs, 0, sizeof(lfs_t));
    struct lfs_config *p_cfg = (struct lfs_config*)(p_lfs + 1);
    lfs_user_data_t* p_user_data = (lfs_user_data_t*)(p_cfg + 1);
    lfs_config_init(p_cfg, p_dev, p_user_data);
    p_user_data->lock = vfs_mutex_lock_create("lfs");
    VFS_ASSERT(p_user_data->lock);

    int32_t ret = lfs_mount(p_lfs, p_cfg);
    if (ret != 0) {
        vfs_mutex_lock_destroy(p_user_data->lock);
        p_user_data->lock = NULL;
        htd_free(p_lfs);
        return ret;
    }
    p_vfs->p_fs_dev = p_lfs;
    return 0;
}

// @brief 卸载
int vfs_lfs_unmount(htd_vfs_t *p_vfs)
{
    lfs_t* p_lfs = (lfs_t*)p_vfs->p_fs_dev;
    VFS_ASSERT(p_lfs);
    lfs_user_data_t* p_user_data = (lfs_user_data_t*)p_lfs->cfg->context;
    VFS_ASSERT(p_user_data);
    
    int32_t ret = lfs_unmount(p_lfs);
    vfs_mutex_lock_destroy(p_user_data->lock);
    p_user_data->lock = NULL;
    htd_free(p_lfs);
    p_vfs->p_fs_dev = NULL;
    return ret;
}

// @brief 格式化文件系统
int vfs_lfs_mk(htd_vfs_t *p_vfs, const void* p_param)
{
    int32_t ret;
    lfs_t* p_lfs = (lfs_t*)p_vfs->p_fs_dev;
    if (p_lfs == NULL) {     // 未挂载, 格式化对应的区域
        p_lfs = htd_malloc(sizeof(lfs_t) + sizeof(struct lfs_config) + sizeof(lfs_user_data_t));
        if (p_lfs == NULL) {
            return -ERROR_CODE_NO_MEM;
        }
        htd_memset(p_lfs, 0, sizeof(lfs_t));
        struct lfs_config *p_cfg = (struct lfs_config*)(p_lfs + 1);
        lfs_user_data_t* p_user_data = (lfs_user_data_t*)(p_cfg + 1);
        lfs_config_init(p_cfg, (lfs_dev_t*)p_param, p_user_data);
        p_user_data->lock = vfs_mutex_lock_create("lfs");
        VFS_ASSERT(p_user_data->lock);
        ret = lfs_format(p_lfs, p_cfg);
        vfs_mutex_lock_destroy(p_user_data->lock);
        htd_free(p_lfs);
        return ret;
    }
    // 已经挂载
    return lfs_format(p_lfs, p_lfs->cfg);
}

// @brief 查看文件系统信息
int vfs_lfs_statfs(htd_vfs_t *p_vfs, htd_vfs_statfs_t *p_buff)
{
    lfs_t* p_lfs = (lfs_t*)p_vfs->p_fs_dev;
    VFS_ASSERT(p_lfs);
    // 填充信息
    uint32_t used_size = lfs_fs_size(p_lfs);
    p_buff->f_blocks = p_lfs->cfg->block_count;
    p_buff->f_bsize = p_lfs->cfg->block_size;
    p_buff->f_bfree = p_lfs->cfg->block_count - used_size;
    return 0;
}

// @brief IO 控制
int vfs_lfs_ioctl(htd_vfs_fd_t *p_fd, int cmd, void *args)
{
    return -ERROR_CODE_NOT_SUPPORTED;
}

// @brief 读数据
ssize_t vfs_lfs_read(htd_vfs_fd_t *p_fd, void *p_buff, size_t buff_len)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_STATE;
    }
    
    lfs_file_t *p_file = (lfs_file_t*)p_fd->data;
    VFS_ASSERT(p_file);
    return lfs_file_read((lfs_t*)p_fd->p_vfs->p_fs_dev, p_file, p_buff, buff_len);
}

// @brief 写数据
ssize_t vfs_lfs_write(htd_vfs_fd_t *p_fd, const void *p_buff, size_t buff_len)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_STATE;
    }

    lfs_file_t *p_file = (lfs_file_t*)p_fd->data;
    VFS_ASSERT(p_file);
    return lfs_file_write((lfs_t*)p_fd->p_vfs->p_fs_dev, p_file, p_buff, buff_len);
}

// @brief 刷新文件
int vfs_lfs_flush(htd_vfs_fd_t *p_fd)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_STATE;
    }
    lfs_file_t *p_file = (lfs_file_t*)p_fd->data;
    VFS_ASSERT(p_file);
    return lfs_file_sync((lfs_t*)p_fd->p_vfs->p_fs_dev, p_file);
}

// @brief 文件位置
htd_vfs_off_t vfs_lfs_lseek(htd_vfs_fd_t *p_fd, htd_vfs_off_t offset)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_STATE;
    }

    lfs_file_t *p_file = (lfs_file_t*)p_fd->data;
    VFS_ASSERT(p_file);
    return lfs_file_seek((lfs_t*)p_fd->p_vfs->p_fs_dev, p_file, offset, LFS_SEEK_SET);
}

// @brief 关闭
int vfs_lfs_close(htd_vfs_fd_t *p_fd)
{
    int32_t ret;
    if (p_fd->type == FT_DIRECTORY) {
        lfs_dir_t* p_dir = (lfs_dir_t*)p_fd->data;
        VFS_ASSERT(p_dir);
        ret = lfs_dir_close((lfs_t*)p_fd->p_vfs->p_fs_dev, p_dir);
    } else {
        lfs_file_t *p_file = (lfs_file_t*)p_fd->data;
        VFS_ASSERT(p_file);
        ret = lfs_file_close((lfs_t*)p_fd->p_vfs->p_fs_dev, p_file);
    }
    return ret;
}

// @brief 打开
int vfs_lfs_open(htd_vfs_fd_t *p_fd)
{
    lfs_t *p_lfs = p_fd->p_vfs->p_fs_dev;
    if (p_lfs == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    
    int32_t ret;
    if (p_fd->flags & O_DIRECTORY) {
        if (p_fd->flags & O_CREAT) {
            ret = lfs_mkdir(p_lfs, p_fd->path);
        }
        lfs_dir_t* p_dir = (lfs_dir_t*)htd_malloc(sizeof(lfs_dir_t));
        if (p_dir == NULL) {
            return -ERROR_CODE_NO_MEM;
        }
        htd_memset(p_dir, 0, sizeof(lfs_dir_t));
        ret = lfs_dir_open(p_lfs, p_dir, p_fd->path);
        if (ret != 0) {
            htd_free(p_dir);
            return ret;
        }
        p_fd->data = p_dir;
        return 0;
    }

    int32_t flags = 0;
    flags |= LFS_O_RDONLY;
    if (p_fd->flags & O_WRONLY) {
        flags |= LFS_O_WRONLY;
    }
    if (p_fd->flags & O_RDWR) {
        flags |= LFS_O_RDWR;
    }
    if (p_fd->flags & O_CREAT) {
        flags |= LFS_O_CREAT;
    }
    if (p_fd->flags & O_EXCL) {
        flags |= LFS_O_EXCL;
    }
    if (p_fd->flags & O_TRUNC) {
        flags |= LFS_O_TRUNC;
    }
    if (p_fd->flags & O_APPEND) {
        flags |= LFS_O_APPEND;
    }
    
    lfs_file_t* p_file = (lfs_file_t*)htd_malloc(sizeof(lfs_file_t));
    if (p_file == NULL) {
        return -ERROR_CODE_NO_MEM;
    }
    ret = lfs_file_open(p_lfs, p_file, p_fd->path, flags);
    if (ret != 0) {
        htd_free(p_file);
        return ret;
    }
    
    p_fd->data = p_file;
    p_fd->size = p_file->ctz.size;
    if (p_fd->flags & O_APPEND){
        p_fd->pos = lfs_file_seek(p_lfs, p_file, p_fd->size, LFS_SEEK_SET);
    }
    return 0;
}

// @brief 查看文件信息
int vfs_lfs_stat(htd_vfs_t *p_vfs, const char *p_path, htd_vfs_stat_t *p_info)
{
    lfs_t *p_lfs = p_vfs->p_fs_dev;
    if (p_lfs == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }

    struct lfs_info fileinfo;
    int32_t ret = lfs_stat(p_lfs, p_path, &fileinfo);
    if (ret == 0) {
        htd_memset(p_info, 0, sizeof(htd_vfs_stat_t));
        p_info->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
        if (fileinfo.type == LFS_TYPE_DIR) {
            p_info->st_mode &= ~S_IFREG;
            p_info->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
        } else {
            p_info->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
        }
        p_info->st_size = fileinfo.size;
    }
    return ret;
}


// @brief 获取目录信息
int vfs_lfs_getdents(htd_vfs_fd_t *p_fd, htd_vfs_dirent_t *p_entry)
{
    if (p_fd->type != FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_PARAM;
    }

    lfs_t* p_lfs = (lfs_t*)p_fd->p_vfs->p_fs_dev;
    lfs_dir_t *p_dir = (lfs_dir_t *)(p_fd->data);
    VFS_ASSERT(p_dir);

    int32_t ret;
    struct lfs_info file_info;
    while (1) {
        ret = lfs_dir_read(p_lfs, p_dir, &file_info);
        if (ret != true || file_info.name[0] == 0) {
            break;
        }
        if (htd_memcmp(file_info.name, ".", 2) == 0 || htd_memcmp(file_info.name, "..", 3) == 0) {
            continue;
        }
        
        p_entry->d_type = (file_info.type == LFS_TYPE_DIR) ? DT_DIR : DT_REG;
        p_entry->d_namlen = htd_strlen(file_info.name);
        p_entry->d_reclen = sizeof(htd_vfs_dirent_t);
        strncpy(p_entry->d_name, file_info.name, sizeof(p_entry->d_name)-1);
        return sizeof(htd_vfs_dirent_t);
    }
    return 0;
}

// @brief 删除
int vfs_lfs_unlink(htd_vfs_t *p_vfs, const char *p_path)
{
    lfs_t *p_lfs = (lfs_t*)p_vfs->p_fs_dev;
    if (p_lfs == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    return lfs_remove(p_lfs, p_path);
}

// @brief 重命名
int vfs_lfs_rename(htd_vfs_t *p_vfs, const char *oldpath, const char *newpath)
{
    lfs_t *p_lfs = (lfs_t*)p_vfs->p_fs_dev;
    if (p_lfs == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    return lfs_rename(p_lfs, oldpath, newpath);
}
HTD_VFS_API_REGISTER(HTD_VFS_TYPE_LITTLE_FS, \
                       vfs_lfs_open, vfs_lfs_close, vfs_lfs_ioctl, \
                       vfs_lfs_read, vfs_lfs_write, vfs_lfs_lseek, NULL, \
                       vfs_lfs_getdents, vfs_lfs_unlink, vfs_lfs_stat, \
                       vfs_lfs_rename, vfs_lfs_mount, vfs_lfs_unmount, vfs_lfs_mk, vfs_lfs_statfs);
#endif /*defined(HTD_VFS_ENABLE) && defined(VFS_USING_LITTLE_FS)*/

