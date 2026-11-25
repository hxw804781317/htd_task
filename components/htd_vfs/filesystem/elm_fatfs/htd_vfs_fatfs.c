// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG              "vfs.fatfs"
#include <htd_vfs_fatfs.h>
#if defined(HTD_VFS_ENABLE) && defined(HTD_VFS_USING_ELM_FATFS)
#include <htd_vfs_process.h>
#include <ff.h>
#include <diskio.h>


//////////////////////////////////////////////////////////////////////
static const elm_fatfs_dev_t *m_fatfs_dev[FF_VOLUMES] = {NULL};

// @brief 注册设备
static int32_t vfs_fatfs_dev_register(const elm_fatfs_dev_t* p_dev)
{
    ASSERT(p_dev->disk_idx < ARRAY_COUNT(m_fatfs_dev));
    if (m_fatfs_dev[p_dev->disk_idx]) {
        return -1;
    }
    print_debug("dev register. disk_idx=%d.", p_dev->disk_idx);
    m_fatfs_dev[p_dev->disk_idx] = p_dev;
    return 0;
}

// @brief 注销
static int32_t vfs_fatfs_dev_unregister(uint8_t disk_idx, bool is_destroy)
{
    ASSERT(disk_idx < ARRAY_COUNT(m_fatfs_dev));
    if (m_fatfs_dev[disk_idx]) {
        print_debug("dev unregister. disk_idx=%d", disk_idx);
        if (is_destroy) {
            if (m_fatfs_dev[disk_idx]->destroy_handle) {
                m_fatfs_dev[disk_idx]->destroy_handle(m_fatfs_dev[disk_idx]);
            }
        }
        m_fatfs_dev[disk_idx] = NULL;
    }
    return 0;
}

// @brief 获取设备
const elm_fatfs_dev_t *vfs_fatfs_dev_get(uint8_t disk_idx)
{
    ASSERT(disk_idx < ARRAY_COUNT(m_fatfs_dev));
    return m_fatfs_dev[disk_idx];
}

// @brief 读扇区数据
// @param sector 第几个扇区，count 多少个扇区
int32_t vfs_fatfs_flash_read(uint8_t disk_idx, uint8_t *buff, uint32_t sector, uint32_t count)
{
    const elm_fatfs_dev_t* p_dev = vfs_fatfs_dev_get(disk_idx);
    ASSERT(p_dev);
    return p_dev->read_handle(p_dev, buff, sector, count);
}

// @brief 写扇区数据
int32_t vfs_fatfs_flash_write(uint8_t disk_idx, const uint8_t *buff, uint32_t sector, uint32_t count)
{
    const elm_fatfs_dev_t* p_dev = vfs_fatfs_dev_get(disk_idx);
    ASSERT(p_dev);
    return p_dev->write_handle(p_dev, buff, sector, count);
}

// @brief 同步
int32_t vfs_fatfs_flash_sync(uint8_t disk_idx)
{
    const elm_fatfs_dev_t* p_dev = vfs_fatfs_dev_get(disk_idx);
    ASSERT(p_dev);
    return p_dev->sync_handle(p_dev);
}



//////////////////////////////////////////////////////////////////////
// @brief 获取盘符
static char* fatfs_logic_path_dup(htd_vfs_t* p_vfs, const char* p_path)
{
    FATFS *p_fs_dev = (FATFS*)p_vfs->p_fs_dev;
    ASSERT(p_fs_dev);

    uint32_t path_len = htd_strlen(p_path) + 8;
    char* p_filepath = (char*)htd_malloc(path_len);
    ASSERT(p_filepath);
    htd_snprintf(p_filepath, path_len, "%d:%s", p_fs_dev->pdrv, p_path);
    return p_filepath;
}

// @brief 挂载
static int vfs_fatfs_mount(htd_vfs_t *p_vfs, const void *data)
{
    elm_fatfs_dev_t* p_dev = (elm_fatfs_dev_t*)data;
    if (p_dev == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    
    FATFS *p_fs_dev = htd_malloc(sizeof(FATFS));
    if (p_fs_dev == NULL) {
        return -ERROR_CODE_NO_MEM;
    }
    htd_memset(p_fs_dev, 0, sizeof(FATFS));
    
    char logic_path[8];
    htd_snprintf(logic_path, sizeof(logic_path), "%d:", p_dev->disk_idx);
    vfs_fatfs_dev_register(p_dev);
    
    FRESULT ret = f_mount(p_fs_dev, logic_path, 1);
    if (ret == FR_OK) {
        FATFS_DIR root;
        char root_path[8];
        htd_snprintf(root_path, sizeof(root_path), "%s/", logic_path);
        ret = f_opendir(&root, root_path);
        if (ret == 0) {
            // 挂载成功
            p_vfs->p_fs_dev = p_fs_dev;
            print_warning("fatfs mount success. %d, logic_path=%s", p_fs_dev->pdrv, logic_path);
            f_closedir(&root);
            return 0;
        }
        
        print_error("opendir failed. root_path=%s, ret=%d", root_path, ret);
        // 卸载掉
        f_mount(NULL, logic_path, 1);
    } 
    
    print_error("mount failed. ret=%d, logic_path=%s", ret, logic_path);
    vfs_fatfs_dev_unregister(p_dev->disk_idx, false);
    htd_free(p_fs_dev);
    return -ret;
}

// @brief 卸载
static int vfs_fatfs_unmount(htd_vfs_t *p_vfs)
{
    FATFS *p_fs_dev = (FATFS *)p_vfs->p_fs_dev;
    ASSERT(p_fs_dev);

    char logic_path[8];
    htd_snprintf(logic_path, sizeof(logic_path), "%d:", p_fs_dev->pdrv);
    
    FRESULT ret = f_unmount(logic_path);
    if (ret == FR_OK) {
        vfs_fatfs_dev_unregister(p_fs_dev->pdrv, true);
        htd_free(p_vfs->p_fs_dev);
        p_vfs->p_fs_dev = NULL;
        return 0;
    }
    return -ret;
}

// @brief 格式化文件系统(必须已经挂载)
static int vfs_fatfs_mk(htd_vfs_t *p_vfs, const void* p_param)
{
    // 格式化参数，该参数和文件系统的碎片化，效率都有关系
    MKFS_PARM opt = {FM_ANY|FM_SFD, 0, 0, 0, 0};

    char logic_path[8];
    char *p_work = (char*)htd_malloc(FF_MAX_SS);
    ASSERT(p_work);

    // 还没挂载的走这里
    if (p_param) {
        const elm_fatfs_dev_t* p_dev = (elm_fatfs_dev_t*)p_param;
        vfs_fatfs_dev_register(p_dev);
        
        htd_snprintf(logic_path, sizeof(logic_path), "%d:", p_dev->disk_idx);
        FRESULT ret = f_mkfs(logic_path, &opt, p_work, FF_MAX_SS);
        htd_free(p_work), p_work = NULL;
        if (ret != FR_OK) {
            print_error("mkfs failed. ret=%d.", ret);
            vfs_fatfs_dev_unregister(p_dev->disk_idx, false);
            return -ret;
        }
        print_warning("mkfs success.");
        return 0;
    }

    // 格式化的时候走这里
    FATFS *p_fs_dev = (FATFS *)p_vfs->p_fs_dev;
    ASSERT(p_fs_dev);
    htd_snprintf(logic_path, sizeof(logic_path), "%d:", p_fs_dev->pdrv);
    FRESULT ret = f_mkfs(logic_path, &opt, p_work, FF_MAX_SS);
    htd_free(p_work), p_work = NULL;
    print_debug("logic_path=%s\r\n", logic_path);
    return -ret;
}

// @brief 查看文件系统信息
static int vfs_fatfs_statfs(htd_vfs_t *p_vfs, htd_vfs_statfs_t *p_statfs)
{
    FATFS *p_fs_dev = (FATFS *)p_vfs->p_fs_dev;
    ASSERT(p_fs_dev);

    DWORD free_clusters = 0;
    char logic_path[8];
    htd_snprintf(logic_path, sizeof(logic_path), "%d:", p_fs_dev->pdrv);
    FRESULT ret = f_getfree(logic_path, &free_clusters, &p_fs_dev);
    if (ret != FR_OK) {
        return -ret;
    }

    // 填充信息
    p_statfs->f_bfree = free_clusters * p_fs_dev->csize;
    p_statfs->f_blocks = (p_fs_dev->n_fatent - 2) * p_fs_dev->csize;
#if FF_MAX_SS != FF_MIN_SS
    p_statfs->f_bsize = p_fs_dev->ssize;
#else
    p_statfs->f_bsize = FF_MAX_SS;
#endif
    htd_print("bfree=%llu, bsize=%u, f_blocks=%llu, csize=%u\r\n", p_statfs->f_bfree, p_statfs->f_bsize, p_statfs->f_blocks, p_fs_dev->csize);
    return 0;
}

// @brief IO 控制
static int vfs_fatfs_ioctl(htd_vfs_fd_t *p_fd, int cmd, void *args)
{
    return -ERROR_CODE_INVALID_STATE;
}

// @brief 读数据
static int vfs_fatfs_read(htd_vfs_fd_t *p_fd, void *p_buff, size_t buff_len)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_STATE;
    }

    FIL *p_file = (FIL*)p_fd->data;
    ASSERT(p_file);

    UINT read_bytes = 0;
    FRESULT ret = f_read(p_file, p_buff, buff_len, &read_bytes);
    if (ret != FR_OK) {
        return -ret;
    }
    return read_bytes;
}

// @brief 写数据
static int vfs_fatfs_write(htd_vfs_fd_t *p_fd, const void *p_buff, size_t buff_len)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_STATE;
    }

    FIL *p_file = (FIL*)p_fd->data;
    ASSERT(p_file);

    UINT write_bytes = 0;
    FRESULT ret = f_write(p_file, p_buff, buff_len, &write_bytes);
    if (ret != FR_OK) {
        return -ret;
    }
    return write_bytes;
}

// @brief 刷新文件
static int vfs_fatfs_flush(htd_vfs_fd_t *p_fd)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_STATE;
    } 
    
    FIL *p_file = (FIL*)p_fd->data;
    ASSERT(p_file);
    FRESULT ret = f_sync(p_file);
    if (ret != FR_OK) {
        return -ret;
    }
    return 0;
}

// @brief 文件位置
static htd_vfs_off_t vfs_fatfs_lseek(htd_vfs_fd_t *p_fd, htd_vfs_off_t offset)
{
    if (p_fd->type == FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_STATE;
    }

    FIL *p_file = (FIL*)p_fd->data;
    ASSERT(p_file);
    FRESULT ret = f_lseek(p_file, offset);
    if (ret != FR_OK) {
        return -ret;
    }
    return p_file->fptr;
}

// @brief 关闭
static int vfs_fatfs_close(htd_vfs_fd_t *p_fd)
{
    FRESULT ret;
    if (p_fd->type == FT_DIRECTORY) {
        FATFS_DIR *p_dir = (FATFS_DIR *)p_fd->data;
        ASSERT(p_dir);
        ret = f_closedir(p_dir);
        if (ret == FR_OK) {
            htd_free(p_dir);
            return 0;
        }
    } else {
        FIL *p_file = (FIL*)p_fd->data;
        ASSERT(p_file);
        ret = f_close(p_file);
        if (ret == FR_OK) {
            htd_free(p_file);
            return 0;
        }
    }
    return -ret;
}

// @brief 打开
static int vfs_fatfs_open(htd_vfs_fd_t *p_fd)
{
    htd_vfs_t *p_vfs = p_fd->p_vfs;
    if (p_vfs == NULL) {
        return -ERROR_CODE_NOT_SUPPORTED;
    }
    
    char* p_filepath = fatfs_logic_path_dup(p_vfs, p_fd->path);
    FRESULT ret;
    if (p_fd->flags & O_DIRECTORY) {
        if (p_fd->flags & O_CREAT) {
            ret = f_mkdir(p_filepath);
            if (ret != FR_OK) {
                htd_free(p_filepath);
                return -ret;
            }
        }
        
        FATFS_DIR* p_dir = (FATFS_DIR*)htd_malloc(sizeof(FATFS_DIR));
        if (p_dir == NULL) {
            htd_free(p_filepath);
            return -ERROR_CODE_NO_MEM;
        }
        ret = f_opendir(p_dir, p_filepath);
        htd_free(p_filepath), p_filepath = NULL;
        if (ret != FR_OK) {
            htd_free(p_dir);
            return -ret;
        }
        p_fd->data = p_dir;
        return 0;
    }
    
    BYTE mode = FA_READ;
    if (p_fd->flags & O_WRONLY) {
        mode |= FA_WRITE;
    }
    if ((p_fd->flags & O_ACCMODE) & O_RDWR) {
        mode |= FA_WRITE;
    }
    if (p_fd->flags & O_CREAT) {
        mode |= FA_OPEN_ALWAYS;
    }
    if (p_fd->flags & O_TRUNC) {
        mode |= FA_CREATE_ALWAYS;
    }
    if (p_fd->flags & O_EXCL) {
        mode |= FA_CREATE_NEW;
    }
    
    FIL* p_file = (FIL*)htd_malloc(sizeof(FIL));
    if (p_file == NULL) {
        htd_free(p_filepath);
        return -ERROR_CODE_NO_MEM;
    }
    ret = f_open(p_file, p_filepath, mode);
    htd_free(p_filepath), p_filepath = NULL;
    if (ret != FR_OK) {
        htd_free(p_file);
        return -ret;
    }

    p_fd->data = p_file;
    p_fd->size = f_size(p_file);
    if (p_fd->flags & O_APPEND){
        f_lseek(p_file, p_fd->size);
        p_fd->pos = p_file->fptr;
    }
    return 0;
}

// @brief 查看文件信息
static int vfs_fatfs_stat(htd_vfs_t *p_vfs, const char *p_path, htd_vfs_stat_t *p_info)
{
    FILINFO file_info;
    char* p_filepath = fatfs_logic_path_dup(p_vfs, p_path);
    FRESULT ret = f_stat(p_filepath, &file_info);
    htd_free(p_filepath), p_filepath = NULL;

    if (ret == FR_OK) {
        p_info->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
        if (file_info.fattrib & AM_DIR) {
            p_info->st_mode &= ~S_IFREG;
            p_info->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
        }
        if (file_info.fattrib & AM_RDO) {
            p_info->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
        }
        p_info->st_size = file_info.fsize;
    }
    return -ret;
}

// @brief 获取目录信息
static int vfs_fatfs_getdents(htd_vfs_fd_t *p_fd, htd_vfs_dirent_t *p_entry)
{
    if (p_fd->type != FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_STATE;
    }

    FILINFO file_info;
    FATFS_DIR *p_dir = (FATFS_DIR *)(p_fd->data);
    ASSERT(p_dir);
    FRESULT ret = f_readdir(p_dir, &file_info);
    if (ret != FR_OK || file_info.fname[0] == 0) {
        return -ret;
    }
    
    char * p_filename = file_info.fname;
    if (file_info.fattrib & AM_DIR) {
        p_entry->d_type = DT_DIR;
    } else {
        p_entry->d_type = DT_REG;
    }
    p_entry->d_namlen = htd_strlen(p_filename);
    p_entry->d_reclen = sizeof(htd_vfs_dirent_t);
    htd_strncpy(p_entry->d_name, p_filename, sizeof(p_entry->d_name)-1);
    return sizeof(htd_vfs_dirent_t);
}

// @brief 删除
static int vfs_fatfs_unlink(htd_vfs_t *p_vfs, const char *p_path)
{
    char* p_filepath = fatfs_logic_path_dup(p_vfs, p_path);
    FRESULT ret = f_unlink(p_filepath);
    htd_free(p_filepath);
    return -ret;
}

// @brief 重命名
static int vfs_fatfs_rename(htd_vfs_t *p_vfs, const char *oldpath, const char *newpath)
{
    char* p_oldfilepath = fatfs_logic_path_dup(p_vfs, oldpath);
    FRESULT ret = f_rename(p_oldfilepath, newpath);
    htd_free(p_oldfilepath);
    return -ret;
}
HTD_VFS_API_REGISTER(HTD_VFS_TYPE_ELM_FATFS, \
                       vfs_fatfs_open, vfs_fatfs_close, vfs_fatfs_ioctl, \
                       vfs_fatfs_read, vfs_fatfs_write, vfs_fatfs_lseek, vfs_fatfs_flush, \
                       vfs_fatfs_getdents, vfs_fatfs_unlink, vfs_fatfs_stat, \
                       vfs_fatfs_rename, vfs_fatfs_mount, vfs_fatfs_unmount, vfs_fatfs_mk, vfs_fatfs_statfs);
#endif /*defined(HTD_VFS_ENABLE) && defined(VFS_USING_ELM_FATFS)*/

