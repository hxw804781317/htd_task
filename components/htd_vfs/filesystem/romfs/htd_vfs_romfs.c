// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include <htd_vfs_process.h>
#include <htd_vfs_file.h>
#include <htd_vfs_romfs.h>


#if defined(HTD_VFS_ENABLE) && defined(HTD_VFS_USING_ROMFS)
//////////////////////////////////////////////////////////////////////
// @brief 挂载
static int vfs_romfs_mount(htd_vfs_t *p_vfs, const void *data)
{
    if (data == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    p_vfs->p_fs_dev = (void*)data;
    return 0;
}

// @brief 卸载
static int vfs_romfs_unmount(htd_vfs_t *p_vfs)
{
    return 0;
}

// @brief IO 控制
static int vfs_romfs_ioctl(htd_vfs_fd_t *p_fd, int cmd, void *args)
{
    return -ERROR_CODE_INVALID_PARAM;
}

// @brief 目录检查
static int check_dirent(vfs_romfs_dirent_t *p_dirent)
{
    if ((p_dirent->type != FSROM_DIRENT_FILE && p_dirent->type != FSROM_DIRENT_DIR) || p_dirent->size == ~0) {
        return -1;
    }
    return 0;
}

// @brief 查找
static vfs_romfs_dirent_t *vfs_romfs_lookup(vfs_romfs_dirent_t *p_root, const char *p_path, size_t *size)
{
    size_t index, found;
    vfs_romfs_dirent_t *p_dirent;
    size_t dirent_size;

    if (check_dirent(p_root) != 0) {
        return NULL;
    }
    if (p_path[0] == '/' && p_path[1] == '\0') {
        *size = p_root->size;
        return p_root;
    }

    /* goto root directy entries */
    p_dirent = (vfs_romfs_dirent_t*)p_root->data;
    dirent_size = p_root->size;

    // 跳过第一层目录
    const char *p_next = htd_vfs_skip_ch(p_path, '/');
    const char *p_next_end = htd_vfs_path_down_one(p_path);
    while (p_dirent != NULL) {
        found = 0;
        for (index = 0; index < dirent_size; index ++) {
            if (check_dirent(&p_dirent[index]) != 0) {
                return NULL;
            }
            if (htd_strlen(p_dirent[index].name) == (p_next_end - p_next) && strncmp(p_dirent[index].name, p_next, (p_next_end - p_next)) == 0) {
                dirent_size = p_dirent[index].size;
                p_next = htd_vfs_skip_ch(p_next_end, '/');
                if (0 == *p_next) {
                    *size = dirent_size;
                    return &p_dirent[index];
                }
                p_next_end = htd_vfs_path_down_one(p_next_end);

                if (p_dirent[index].type == FSROM_DIRENT_DIR) {
                    p_dirent = (vfs_romfs_dirent_t *)p_dirent[index].data;
                    found = 1;
                    break;
                } else {
                    if (p_next != NULL) {
                        break; 
                    }
                    return &p_dirent[index];
                }
            }
        }

        if (!found) {
            break;
        }
    }

    return NULL;
}

// @brief 读数据
static ssize_t vfs_romfs_read(htd_vfs_fd_t *p_fd, void *p_buff, size_t buff_len)
{
    vfs_romfs_dirent_t *p_dirent = (vfs_romfs_dirent_t *)p_fd->data;
    ASSERT(p_dirent != NULL);

    if (check_dirent(p_dirent) != 0) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    htd_memcpy(p_buff, p_dirent->data + p_fd->pos, buff_len);
    return buff_len;
}

// @brief 偏移
static htd_vfs_off_t vfs_romfs_lseek(htd_vfs_fd_t *p_fd, htd_vfs_off_t offset)
{
    if (offset <= p_fd->size) {
        p_fd->pos = offset;
        return p_fd->pos;
    }
    return -ERROR_CODE_INVALID_PARAM;
}

// @brief 关闭文件
static int vfs_romfs_close(htd_vfs_fd_t *p_fd)
{
    p_fd->data = NULL;
    return ERROR_CODE_SUCCESS;
}

// @brief 打开文件
static int vfs_romfs_open(htd_vfs_fd_t *p_fd)
{
    htd_vfs_t *p_vfs = p_fd->p_vfs;
    vfs_romfs_dirent_t *p_root = (vfs_romfs_dirent_t *)p_vfs->p_fs_dev;
    if (check_dirent(p_root) != 0) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    if (p_fd->flags & (O_CREAT | O_WRONLY | O_APPEND | O_TRUNC | O_RDWR)) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    
    size_t size;
    vfs_romfs_dirent_t *p_dirent = vfs_romfs_lookup(p_root, p_fd->path, &size);
    if (p_dirent == NULL) {
        return -ERROR_CODE_NOT_SUPPORTED;
    }

    /* entry is a directory p_fd type */
    if (p_dirent->type == FSROM_DIRENT_DIR) {
        if (!(p_fd->flags & O_DIRECTORY)) {
            return -ERROR_CODE_NOT_SUPPORTED;
        }
    } else {
        if (p_fd->flags & O_DIRECTORY) {
            return -ERROR_CODE_NOT_SUPPORTED;
        }
    }
    p_fd->data = p_dirent;
    p_fd->size = size;
    p_fd->pos = 0;
    return 0;
}

// @brief 文件信息
static int vfs_romfs_stat(htd_vfs_t *p_vfs, const char *p_path, htd_vfs_stat_t *p_info)
{
    size_t size;
    vfs_romfs_dirent_t *p_root = (vfs_romfs_dirent_t *)p_vfs->p_fs_dev;
    vfs_romfs_dirent_t *p_dirent = vfs_romfs_lookup(p_root, p_path, &size);
    if (p_dirent == NULL) {
        return -ERROR_CODE_NOT_SUPPORTED;
    }
    
    p_info->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
    if (p_dirent->type == FSROM_DIRENT_DIR) {
        p_info->st_mode &= ~S_IFREG;
        p_info->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
    }
    p_info->st_size = p_dirent->size;
    p_info->st_mtime = 0;
    return 0;
}

// @brief 迭代目录
static int vfs_romfs_getdents(htd_vfs_fd_t *p_fd, htd_vfs_dirent_t *p_entry)
{
    vfs_romfs_dirent_t* p_dirent = (vfs_romfs_dirent_t *)p_fd->data;
    if (check_dirent(p_dirent) != 0) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    ASSERT(p_dirent->type == FSROM_DIRENT_DIR);
    p_dirent = (vfs_romfs_dirent_t*)p_dirent->data;
    
    const char* p_name = p_dirent[p_fd->pos].name;
    if (p_dirent[p_fd->pos].type == FSROM_DIRENT_DIR) {
        p_entry->d_type = DT_DIR;
    } else {
        p_entry->d_type = DT_REG;
    }
    p_entry->d_namlen = htd_strlen(p_name);
    p_entry->d_reclen = (uint16_t)sizeof(htd_vfs_dirent_t);
    htd_memcpy(p_entry->d_name, p_name, p_entry->d_namlen + 1);
    p_fd->pos++;
    return sizeof(htd_vfs_dirent_t);
}
HTD_VFS_API_REGISTER(HTD_VFS_TYPE_ROMFS, \
                       vfs_romfs_open, vfs_romfs_close, vfs_romfs_ioctl, \
                       vfs_romfs_read, NULL, vfs_romfs_lseek, NULL, \
                       vfs_romfs_getdents, NULL, vfs_romfs_stat, \
                       NULL, vfs_romfs_mount, vfs_romfs_unmount, NULL, NULL);
#endif /*defined(HTD_VFS_ENABLE) && defined(VFS_USING_ROMFS)*/

