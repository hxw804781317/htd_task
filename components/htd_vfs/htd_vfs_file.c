// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG              "vfs.file"
#include "htd_vfs_port.h"
#include "htd_vfs_process.h"
#include "htd_vfs_file.h"

static htd_vfs_fd_t *m_fd_table[HTD_VFS_FD_MAX] = {NULL};

// @brief 申请一个文件描述符
static htd_vfs_fd_t* vfs_fd_alloc(void)
{
    htd_vfs_fd_t* p_fd = NULL;
    htd_vfs_lock();
    for (uint32_t i=0; i<ARRAY_COUNT(m_fd_table); ++i) {
        if (m_fd_table[i] == NULL) {
            m_fd_table[i] = htd_malloc(sizeof(htd_vfs_fd_t));
            ASSERT(m_fd_table[i]);
            htd_memset(m_fd_table[i], 0, sizeof(htd_vfs_fd_t));
            p_fd = m_fd_table[i];
            p_fd->fd = i + VFS_FD_OFFSET;
            break;
        }
    }
    htd_vfs_unlock();
    return p_fd;
}

// @brief 释放文件描述符
static void vfs_fd_free(htd_vfs_fd_t* p_fd)
{
    int i = p_fd->fd - VFS_FD_OFFSET;
    htd_vfs_lock();
    if (i >= 0 && i < ARRAY_COUNT(m_fd_table) && m_fd_table[i] == p_fd) {
        if (p_fd->path) {
            htd_free(p_fd->path);
        }
        htd_free(m_fd_table[i]);
        m_fd_table[i] = NULL;
    } else {
        for (i=0; i<ARRAY_COUNT(m_fd_table); ++i) {
            if (m_fd_table[i] && m_fd_table[i] == p_fd) {
                if (p_fd->path) {
                    htd_free(p_fd->path);
                }
                htd_free(m_fd_table[i]);
                m_fd_table[i] = NULL;
                break;
            }
        }
    }
    htd_vfs_unlock();
}

// @brief 文件id获取文件句柄
static htd_vfs_fd_t* vfs_fd_get(int file_id)
{
    file_id -= VFS_FD_OFFSET;
    if (file_id >= 0 && file_id < ARRAY_COUNT(m_fd_table)) {
        return m_fd_table[file_id];
    }
    return NULL;
}

// @brief 判断文件是否已经打开
int htd_vfs_file_is_open(const char *p_pathname)
{
    char* p_fullpath = htd_vfs_path_linux_standard(NULL, p_pathname);
    if (NULL == p_fullpath) {
        return -ERROR_CODE_NO_SPACE;
    }

    htd_vfs_t* p_vfs = htd_vfs_find(p_fullpath);
    if (p_vfs == NULL) {
        htd_free(p_fullpath);
        return -1;
    }
    
    char *p_mount_path;
    if (p_vfs->p_path[0] == '/' && p_vfs->p_path[1] == '\0') {
        p_mount_path = p_fullpath;
    } else {
        p_mount_path = p_fullpath + htd_strlen(p_vfs->p_path);
    }

    htd_vfs_fd_t* p_fd;
    htd_vfs_lock();
    for (uint32_t i=0; i<ARRAY_COUNT(m_fd_table); ++i) {
        p_fd = m_fd_table[i];
        if (p_fd == NULL || p_fd->path == NULL) {
            continue;
        }
        if (p_fd->p_vfs == p_vfs && htd_strcmp(p_fd->path, p_mount_path) == 0) {
            htd_free(p_fullpath);
            htd_vfs_unlock();
            return 0;
        }
    }
    htd_vfs_unlock();
    htd_free(p_fullpath);
    return -1;
}

// @brief 打开文件
// @return 成功返回对应的文件描述符，失败返回NULL
static htd_vfs_fd_t* file_open(const char *p_path, int flags)
{
    char* p_fullpath = htd_vfs_path_linux_standard(NULL, p_path);
    if (p_fullpath == NULL) {
        return NULL;
    }

    // 查找对应的文件系统
    htd_vfs_t *p_vfs = htd_vfs_find(p_fullpath);
    if (p_vfs == NULL) {
        htd_free(p_fullpath); 
        htd_print("fs was not found.\r\n");
        return NULL;
    }

    htd_vfs_fd_t *p_fd = vfs_fd_alloc();
    if (p_fd == NULL) {
        htd_free(p_fullpath); 
        return NULL;
    }
    
    //vfs_print("open in filesystem: %s\n", p_fs->p_name);
    p_fd->magic = VFS_FD_MAGIC;
    p_fd->p_vfs = p_vfs; 
    p_fd->type = FT_REGULAR;
    p_fd->flags = flags;
    p_fd->size = 0;
    p_fd->pos = 0;
    p_fd->data = NULL;

    // 相对文件系统的路径
    const char* p_dir = htd_vfs_subdir(p_vfs->p_path, p_fullpath);
    if (p_dir == NULL) {
        p_fd->path = htd_vfs_strdup("/");
    } else {
        p_fd->path = htd_vfs_strdup(p_dir);
    }
    htd_free(p_fullpath);

    // 打开文件
    if (p_vfs->f_open) {
        if (p_vfs->f_open(p_fd) < 0) {
            vfs_fd_free(p_fd);
            htd_print("vfs open failed.\r\n");
            return NULL;
        }
    }
    
    p_fd->flags |= HTD_F_OPEN;              // 标记文件已经打开
    if (flags & O_DIRECTORY) {
        p_fd->type = FT_DIRECTORY;
    }
    return p_fd;
}

// @brief 打开模式转标记
static int32_t vfs_fopen_mode2flags(const char* mode)
{
    int32_t flags = O_BINARY|O_RDONLY;
    bool w = 0;
    bool a = 0;
    while (*mode) {
        char ch = *mode++;
        if (ch == 'w') {
            w = 1;
        } else if (ch == 'a') {
            a = 1;
        }
    }
    if (w) {
        flags |= O_CREAT|O_RDWR|O_TRUNC;
    }
    if (a) {
        flags |= O_APPEND|O_CREAT;
        flags &= ~O_TRUNC;
    }
    return flags;
}

// @brief 打开文件(对应C标准函数fopen)
// @return 成功返回对应的文件描述符，失败返回NULL
htd_vfs_fd_t* htd_vfs_fopen(const char *p_path, const char* mode)
{
    int32_t flags = vfs_fopen_mode2flags(mode);
    return file_open(p_path, flags);
}

// @brief 关闭文件(对应C标准函数fclose)
// @return 成功返回0，失败返回小于0的错误码
int htd_vfs_fclose(htd_vfs_fd_t *p_fd)
{
    if (p_fd == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    int result = 0;
    if (p_fd->p_vfs->f_close != NULL) {
        result = p_fd->p_vfs->f_close(p_fd);
    }
    vfs_fd_free(p_fd);
    return result;
}

// @brief 读文件数据(对应C标准函数fread)
// @return 返回读取到的数据长度，失败返回小于0的错误码
ssize_t htd_vfs_fread(htd_vfs_fd_t *p_fd, void *p_buff, size_t buff_size)
{
    if (p_fd == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    if (p_fd->pos >= p_fd->size) {
        //vfs_print("fread failed. pos=%d, size=%d\r\n", p_fd->pos, p_fd->size);
        return 0;
    }

    // 计算最多只能读取的数据长度
    size_t length = p_fd->size - p_fd->pos;
    if (buff_size > length) {
        buff_size = length;
    }
    
    ssize_t result = 0;
    if (p_fd->p_vfs->f_read) {
        result = p_fd->p_vfs->f_read(p_fd, p_buff, buff_size);
        if (result > 0) {
            p_fd->pos += result;
        }
    }
    return result;
}

// @brief 写文件数据(对应C标准函数fwrite)
// @return 成功返回写入的数据长度，失败返回小于0的错误码
ssize_t htd_vfs_fwrite(htd_vfs_fd_t *p_fd, const void *p_buff, size_t buff_len)
{
    if (p_fd == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    if (p_fd->p_vfs->f_write == NULL) {
        return -ERROR_CODE_INVALID_STATE;
    }
    
    ssize_t result = p_fd->p_vfs->f_write(p_fd, p_buff, buff_len);
    if (result > 0) {
        p_fd->pos += result;
        if (p_fd->pos > p_fd->size) {
            p_fd->size = p_fd->pos;
        }
    }
    return result;
}

// @brief 刷新文件缓冲数据(对应C标准函数fflush)
int htd_vfs_fflush(htd_vfs_fd_t *p_fd)
{
    if (p_fd == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    if (p_fd->p_vfs->f_flush == NULL) {
        return -ERROR_CODE_INVALID_STATE;
    }
    return p_fd->p_vfs->f_flush(p_fd);
}

// @brief 重定位文件位置(对应C标准函数fseek)
htd_vfs_off_t htd_vfs_fseek(htd_vfs_fd_t *p_fd, htd_vfs_off_t offset, int whence)
{
    if (p_fd == NULL) {
        return -1;
    }
    
    switch (whence) {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            offset += p_fd->pos;
            break;
        case SEEK_END:
            offset += p_fd->size;
            break;
        default:
            return -1;
    }
    if (offset < 0) {
        return -1;
    }
    
    if (p_fd->p_vfs->f_lseek == NULL) {
        return -ERROR_CODE_INVALID_STATE;
    }
    htd_vfs_off_t result = p_fd->p_vfs->f_lseek(p_fd, offset);
    if (result >= 0) {
        p_fd->pos = result;
     }
    return result;
}

// @brief 返回当前文件的位置(对应C标准函数ftell)
htd_vfs_off_t htd_vfs_ftell(htd_vfs_fd_t* p_fd)
{
    return p_fd->pos;
}

// @brief 判断是否已经达到文件末尾(对应C标准函数feof)
bool htd_vfs_feof(htd_vfs_fd_t* p_fd)
{
    if (p_fd->pos >= p_fd->size) {
        return true;
    }
    return false;
}

// @brief 文件大小
htd_vfs_off_t htd_vfs_fsize(htd_vfs_fd_t* p_fd)
{
    return p_fd->size;
}

// @brief 删除文件
int htd_vfs_remove(const char *path)
{
    char *p_fullpath = htd_vfs_path_linux_standard(NULL, path);
    if (p_fullpath == NULL) {
        return -ERROR_CODE_NO_SPACE;
    }

    const char* p_sub;
    int result = 0;
    do {
        htd_vfs_t* p_vfs = htd_vfs_find(p_fullpath);
        if (p_vfs == NULL) {
            result = -ERROR_CODE_NOT_SUPPORTED;
            break;
        }
        if (htd_vfs_file_is_open(p_fullpath) == 0) {
            result = -ERROR_CODE_BUSY; 
            break;
        }
        if (p_vfs->f_unlink == NULL) {
            result = -ERROR_CODE_INVALID_STATE;
            break;
        }

        p_sub = htd_vfs_subdir(p_vfs->p_path, p_fullpath);
        if (p_sub == NULL) {
            result = p_vfs->f_unlink(p_vfs, "/");
        } else {
            result = p_vfs->f_unlink(p_vfs, p_sub);
        }
    } while (0);
    
    htd_free(p_fullpath);
    return result;
}

// @brief 重命名(对应C标准函数rename)
int htd_vfs_rename(const char *p_oldpath, const char *p_newpath)
{
    char* p_oldfullpath = htd_vfs_path_linux_standard(NULL, p_oldpath);
    if (p_oldfullpath == NULL) {
        return -ERROR_CODE_NO_SPACE;
    }
    char* p_newfullpath = htd_vfs_path_linux_standard(NULL, p_newpath);
    if (p_newfullpath == NULL) {
        htd_free(p_oldfullpath);
        return -ERROR_CODE_NO_SPACE;
    }

    int result = 0;
    htd_vfs_t *p_oldfs = htd_vfs_find(p_oldfullpath);
    htd_vfs_t *p_newfs = htd_vfs_find(p_newfullpath);
    if (p_oldfs == p_newfs) {
        if (p_oldfs->f_rename == NULL) {
            result = -ERROR_CODE_INVALID_STATE;
        } else {
            result = p_oldfs->f_rename(p_oldfs, htd_vfs_subdir(p_oldfs->p_path, p_oldfullpath), htd_vfs_subdir(p_newfs->p_path, p_newfullpath));
        }
    } else {
        result = -ERROR_CODE_NOT_FOUND;
    }

    htd_free(p_oldfullpath);
    htd_free(p_newfullpath);
    return result;
}

// @brief 把文件指针移到开始处(对应C标准函数rewind)
void htd_vfs_rewind(htd_vfs_fd_t* p_fd)
{
    htd_vfs_fseek(p_fd, 0, SEEK_SET);
}

// @brief 打开文件(对应linux函数open)
int htd_vfs_file_open(const char *p_path, int flags)
{
    htd_vfs_fd_t* p_fd = file_open(p_path, flags);
    if (p_fd) {
        return p_fd->fd;
    }
    return -1;
}

// @brief 关闭文件(对应 linux 函数 close)
int htd_vfs_file_close(int file_id)
{
    htd_vfs_fd_t *p_fd = vfs_fd_get(file_id);
    return htd_vfs_fclose(p_fd);
}

// @brief 读文件数据(对应 linux 函数 read)
ssize_t htd_vfs_file_read(int file_id, void *p_buff, size_t buff_size)
{
    htd_vfs_fd_t *p_fd = vfs_fd_get(file_id);
    return htd_vfs_fread(p_fd, p_buff, buff_size);
}

// @brief 写文件数据(对应 linux 函数 write)
ssize_t htd_vfs_file_write(int file_id, const void *p_buff, size_t buff_len)
{
    htd_vfs_fd_t *p_fd = vfs_fd_get(file_id);
    return htd_vfs_fwrite(p_fd, p_buff, buff_len);
}

// @brief 文件io控制(对应 linux 函数 ioctl)
// @return 成功返回0，失败返回小于0的错误码
int htd_vfs_file_ioctl(int file_id, int cmd, void *args)
{
    htd_vfs_fd_t *p_fd = vfs_fd_get(file_id);
    if (p_fd == NULL) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    
    if (p_fd->type == FT_REGULAR) {
        switch (cmd) {
            case F_GETFL: {
                return p_fd->flags; 
            }
            case F_SETFL: {
                int flags = (int)args;
                int mask  = O_NONBLOCK | O_APPEND;
                flags &= mask;
                p_fd->flags &= ~mask;
                p_fd->flags |= flags;
                return 0;
            }
        }
    }
    if (p_fd->p_vfs->f_ioctl) {
        return p_fd->p_vfs->f_ioctl(p_fd, cmd, args);
    }
    return -ERROR_CODE_INVALID_STATE;
}

// @brief 文件控制(对应 linux 函数 fcntl)
int htd_vfs_file_fcntl(int file_id, int cmd, ...)
{
    void *arg;
    va_list ap;
    va_start(ap, cmd);
    arg = va_arg(ap, void*);
    va_end(ap);
    return htd_vfs_file_ioctl(file_id, cmd, arg);
}

// @brief 获取目录信息
int htd_vfs_file_getdents(int file_id, htd_vfs_dirent_t *p_dir)
{
    htd_vfs_fd_t *p_fd = vfs_fd_get(file_id);
    if (p_fd == NULL || p_fd->type != FT_DIRECTORY) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    if (p_fd->p_vfs->f_getdents) {
        return p_fd->p_vfs->f_getdents(p_fd, p_dir);
    }
    return -ERROR_CODE_INVALID_STATE;
}

// @brief 删除文件
int htd_vfs_file_unlink(const char *path)
{
    return htd_vfs_remove(path);
}

// @brief 刷新文件缓冲数据(对应 linux 函数 fsync)
int htd_vfs_file_fsync(int file_id)
{
    htd_vfs_fd_t *p_fd = vfs_fd_get(file_id);
    return htd_vfs_fflush(p_fd);
}

// @brief 重定位文件位置(对应 linux 函数 lseek)
htd_vfs_off_t htd_vfs_file_lseek(int file_id, htd_vfs_off_t offset, int whence)
{
    htd_vfs_fd_t *p_fd = vfs_fd_get(file_id);
    return htd_vfs_fseek(p_fd, offset, whence);
}

// @brief 获取文件信息
int htd_vfs_file_stat(const char *path, htd_vfs_stat_t *p_info)
{
    char *p_fullpath = htd_vfs_path_linux_standard(NULL, path);
    if (p_fullpath == NULL) {
        return -ERROR_CODE_NO_SPACE;
    }

    htd_vfs_t* p_vfs = htd_vfs_find(p_fullpath);
    if (p_vfs == NULL) {
        htd_print("has not found filesystem. %s.\r\n", p_fullpath);
        htd_free(p_fullpath);
        return -ERROR_CODE_NOT_SUPPORTED; 
    }

    int result = 0;
    const char* p_subdir = htd_vfs_subdir(p_vfs->p_path, p_fullpath);
    if ((p_fullpath[0] == '/' && p_fullpath[1] == '\0') || p_subdir == NULL) {
        p_info->st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
        p_info->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
        p_info->st_size = 0;
        p_info->st_mtime = 0;
    } else {
        result = -ERROR_CODE_INVALID_STATE;
        if (p_vfs->f_stat) {
            result = p_vfs->f_stat(p_vfs, p_subdir, p_info);
        }
    }
    htd_free(p_fullpath);
    return result;
}

// @brief 获取文件信息(对应 linux 函数 fstat)
int htd_vfs_file_fstat(int file_id, htd_vfs_stat_t *p_info)
{
    htd_vfs_fd_t* p_fd = vfs_fd_get(file_id);
    if (p_fd == NULL) {
        return -1;
    }
    /* it's the root directory */
    p_info->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
    if (p_fd->type == FT_DIRECTORY) {
        p_info->st_mode &= ~S_IFREG;
        p_info->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
    }
    p_info->st_size = p_fd->size;
    p_info->st_mtime = 0;
    return 0;
}

// @brief 重置文件大小
int htd_vfs_file_ftruncate(int file_id, htd_vfs_off_t length)
{
    htd_vfs_fd_t *p_fd = vfs_fd_get(file_id);
    if (p_fd == NULL || p_fd->type != FT_REGULAR || length < 0) {
        return -ERROR_CODE_INVALID_PARAM;
    }
    if (p_fd->p_vfs->f_ioctl == NULL) {
        return -ERROR_CODE_INVALID_STATE;
    }
    int result = p_fd->p_vfs->f_ioctl(p_fd, VFS_F_TRUNCATE, (void*)&length);
    if (result == 0) {
        p_fd->size = length;
        if (p_fd->pos > p_fd->size) {
            p_fd->pos = p_fd->size;
        }
    }
    return result;
}

// @brief 显示文件列表
void htd_vfs_ls(const char *p_pathname)
{
    char *path;
    if (p_pathname == NULL) {
        path = htd_vfs_strdup(htd_vfs_work_dir_get());
        if (path == NULL) {
            return ; 
        }
    } else {
        path = (char*)p_pathname;
    }

    //vfs_print("path=%s\n", path);
    int fd = htd_vfs_file_open(path, O_DIRECTORY);
    if (fd < 0) {
        htd_print("No such directory.\r\n");
        if (p_pathname == NULL) {
            htd_free(path);
        }
        return;
    }

    int32_t ret;
    htd_vfs_stat_t stat;
    htd_vfs_dirent_t dirent;
    const char* p_subdir;
    char* p_fullpath;
    htd_vfs_fd_t* p_fd = vfs_fd_get(fd);
    htd_vfs_t* p_vfs = p_fd->p_vfs;
    
    htd_print("\033[0;33mDir %s:\r\n\033[0m", path);
    while (1) {
        htd_memset(&dirent, 0, sizeof(htd_vfs_dirent_t));
        ret = htd_vfs_file_getdents(fd, &dirent);
        if (ret <= 0) {
            break;
        }
        if (htd_memcmp(dirent.d_name, ".", 2) == 0 || htd_memcmp(dirent.d_name, "..", 3) == 0) {
            continue;
        }
        
        htd_memset(&stat, 0, sizeof(htd_vfs_stat_t));
        p_fullpath = htd_vfs_path_linux_standard(path, dirent.d_name);
        ASSERT(p_fullpath);
        p_subdir = htd_vfs_subdir(p_vfs->p_path, p_fullpath);
        if (p_subdir == NULL) {
            htd_free(p_fullpath);
            continue;
        }

        if (p_vfs->f_stat) {
            ret = p_vfs->f_stat(p_vfs, p_subdir, &stat);
        } else {
            ret = -1;
        }
        if (ret == 0) {
            if (S_ISDIR(stat.st_mode)) {
                htd_print("\t\033[0;35m%-20s\r\n\033[0m", dirent.d_name);
            } else {
                htd_print("\t%-20s%-25lu\r\n", dirent.d_name, (unsigned long)stat.st_size);
            }
        } else {
            htd_print("\tBAD file: %s\r\n", dirent.d_name);
        }
        htd_free(p_fullpath);
    }
    
    htd_vfs_file_close(fd);
    if (p_pathname == NULL) {
        htd_free(path);
    }
}

// @brief 显示文件内容
void htd_vfs_file_cat(const char *p_filename)
{
    // 打开文件
    htd_vfs_fd_t* p_fd = htd_vfs_fopen(p_filename, "r");
    if (p_fd == NULL) {
        htd_print("Open %s failed\r\n", p_filename);
        return;
    }

    // 读文件数据后打印显示
    int32_t length;
    char buff[65];
    while (!htd_vfs_feof(p_fd)) {
        length = htd_vfs_fread(p_fd, buff, sizeof(buff) - 1);
        if (length > 0) {
            buff[length] = 0;
            htd_print("%s", buff);
        } 
    }
    htd_print("\r\n");

    // 关闭文件
    htd_vfs_fclose(p_fd);
}

// @brief 获取文件大小
uint32_t htd_vfs_file_size_get(const char* filepath)
{
    uint32_t fsize;
    htd_vfs_fd_t* p_fd = file_open(filepath, O_BINARY|O_RDONLY);
    if (NULL == p_fd) {
        return 0;
    }
    fsize = p_fd->size;
    htd_vfs_fclose(p_fd);
    return fsize;
}

// @brief 拷贝一个文件
static void file_copy(const char *p_spath, const char *p_dpath)
{
    const uint32_t buff_len = 4096;
    uint8_t* p_buff = (uint8_t *)htd_malloc(buff_len);
    if (p_buff == NULL) {
        htd_print("out of memory\r\n");
        return;
    }

    // 打开源文件
    htd_vfs_fd_t *p_sfd = htd_vfs_fopen(p_spath, "r");
    if (NULL == p_sfd) {
        htd_free(p_buff);
        htd_print("Read %s failed\r\n", p_spath);
        return;
    }
    
    // 打开目标文件
    htd_vfs_fd_t* p_dfd = htd_vfs_fopen(p_dpath, "w");
    if (NULL == p_dfd) {
        htd_free(p_buff);
        htd_vfs_fclose(p_sfd);
        htd_print("Write %s failed\r\n", p_dpath);
        return;
    }

    // 拷贝源文件数据到目标文件中
    while (1) {
        int32_t read_bytes = htd_vfs_fread(p_sfd, p_buff, buff_len);
        if (read_bytes <= 0) {
            break;
        }
        int length = htd_vfs_fwrite(p_dfd, p_buff, read_bytes);
        if (length != read_bytes) {
            htd_print("Write file data failed, errno=%d\r\n", length);
            break;
        }
    }

    // 关闭文件
    htd_vfs_fclose(p_dfd);
    htd_vfs_fclose(p_sfd);
    htd_free(p_buff);
}

// @brief 递归拷贝目录
static void dir_copy(const char *src, const char *dst)
{
    int fd = htd_vfs_file_open(src, O_DIRECTORY);
    if (fd < 0) {
        htd_print("open %s failed\r\n", src);
        return ;
    }

    htd_vfs_dirent_t dirent;
    htd_vfs_stat_t stat;
    while (1) {
        htd_memset(&dirent, 0, sizeof(htd_vfs_dirent_t));
        int length = htd_vfs_file_getdents(fd, &dirent);
        if (length <= 0) {
            break;
        }
        
        if (htd_strcmp(dirent.d_name, "..") == 0 || htd_strcmp(dirent.d_name, ".") == 0) {
            continue;
        }

        char* src_entry_full = htd_vfs_path_linux_standard(src, dirent.d_name);
        ASSERT(src_entry_full);
        char* dst_entry_full = htd_vfs_path_linux_standard(dst, dirent.d_name);
        ASSERT(dst_entry_full);
        htd_memset(&stat, 0, sizeof(htd_vfs_stat_t));
        if (htd_vfs_file_stat(src_entry_full, &stat) != 0) {
            htd_print("open file: %s failed\r\n", dirent.d_name);
            continue;
        }
        if (S_ISDIR(stat.st_mode)) {
            htd_vfs_mkdir(dst_entry_full);
            dir_copy(src_entry_full, dst_entry_full);
        } else {
            file_copy(src_entry_full, dst_entry_full);
        }
        htd_free(src_entry_full);
        htd_free(dst_entry_full);
    }

    htd_vfs_file_close(fd);
}

// @brief 拷贝
void htd_vfs_copy(const char *src, const char *dst)
{
    #define FLAG_SRC_TYPE      0x03
    #define FLAG_SRC_IS_DIR    0x01
    #define FLAG_SRC_IS_FILE   0x02
    #define FLAG_SRC_NON_EXSIT 0x00
    #define FLAG_DST_TYPE      0x0C
    #define FLAG_DST_IS_DIR    0x04
    #define FLAG_DST_IS_FILE   0x08
    #define FLAG_DST_NON_EXSIT 0x00

    htd_vfs_stat_t stat;
    if (htd_vfs_file_stat(src, &stat) < 0) {
        htd_print("copy failed, bad %s\r\n", src);
        return;
    }

    uint32_t flag = 0;
    if (S_ISDIR(stat.st_mode)) {
        flag |= FLAG_SRC_IS_DIR;
    } else {
        flag |= FLAG_SRC_IS_FILE;
    }

    if (htd_vfs_file_stat(dst, &stat) < 0) {
        flag |= FLAG_DST_NON_EXSIT;
    } else {
        if (S_ISDIR(stat.st_mode)) {
            flag |= FLAG_DST_IS_DIR;
        } else {
            flag |= FLAG_DST_IS_FILE;
        }
    }

    //检查类型是否一致
    if ((flag & FLAG_SRC_IS_DIR) && (flag & FLAG_DST_IS_FILE)) {
        htd_print("cp faild, cp dir to file is not permitted!\r\n");
        return ;
    }

    //拷贝
    char *p_dst_filename;
    if (flag & FLAG_SRC_IS_FILE) {
        if (flag & FLAG_DST_IS_DIR) {
            p_dst_filename = htd_vfs_path_linux_standard(dst, htd_vfs_file_basename_get(src));
            ASSERT(p_dst_filename);
            file_copy(src, p_dst_filename);
            htd_free(p_dst_filename);
        } else {
            file_copy(src, dst);
        }
    } else {    //flag & FLAG_SRC_IS_DIR
        if (flag & FLAG_DST_IS_DIR) {
            p_dst_filename = htd_vfs_path_linux_standard(dst, htd_vfs_file_basename_get(src));
            ASSERT(p_dst_filename);
            htd_vfs_mkdir(p_dst_filename);
            dir_copy(src, p_dst_filename);
            htd_free(p_dst_filename);
        }  else if ((flag & FLAG_DST_TYPE) == FLAG_DST_NON_EXSIT) {
            htd_vfs_mkdir(dst);
            dir_copy(src, dst);
        } else {
            dir_copy(src, dst);
        }
    }
}

// @brief 创建目录
int htd_vfs_mkdir(const char *p_path)
{
    htd_vfs_fd_t* p_fd = file_open(p_path, O_DIRECTORY|O_CREAT);
    if (p_fd == NULL) {
        return -1;
    }
    htd_vfs_fclose(p_fd);
    return 0;
}

// @brief 创建对应的文件路径
bool htd_vfs_mkdir_pp(const char* p_path)
{
    const char *p_ptr, *p_start;
    char dir[64];
    uint8_t len;

    if (p_path[0] != '/') {
        return false;
    }
    
    if (0 == access(p_path, 0)) {
        return true;
    }
    
    p_start = p_path + 1;
    while (1) {
        p_ptr = strchr(p_start, '/');
        if (p_ptr == NULL) {
            break;
        }
        
        len = p_ptr - p_path;
        htd_memcpy(dir, p_path, len);
        dir[len] = 0;
        htd_vfs_mkdir(dir);
        p_start = p_ptr + 1;
    }
    return true;
}

// @brief 删除文件夹以及文件夹里面所有文件
void htd_vfs_rmdir_rf(const char* p_path)
{
    DIR *p_dir;
    char *p_full_path = NULL;
    htd_vfs_dirent_t *p_dirinfo;
    
    p_dir = opendir(p_path);
    if (NULL == p_dir) {
        htd_print("open dir failed. path=%s\r\n", p_path);
        htd_vfs_file_unlink(p_path);
        return;
    }

    while (1) {
        p_dirinfo = readdir(p_dir);
        if (NULL == p_dirinfo) {
            break;
        }
        if (htd_strcmp(p_dirinfo->d_name, ".") == 0 || htd_strcmp(p_dirinfo->d_name, "..") == 0) {
            continue;
        }
        if (p_dirinfo->d_type == DT_UNKNOWN) {
            continue;
        }
        
        p_full_path = htd_vfs_path_linux_standard(p_path, p_dirinfo->d_name);
        if (p_full_path == NULL) {
            continue;
        }
        htd_print("rmdir: type=%d, d_name=%s, name_len=%d\r\n", p_dirinfo->d_type, p_dirinfo->d_name, p_dirinfo->d_namlen);
        if (p_dirinfo->d_type != DT_DIR) {
            htd_print("rm file: %s\r\n", p_full_path);
            htd_vfs_file_unlink(p_full_path);
        } else {
            htd_print("rm dir: %s\r\n", p_full_path);
            htd_vfs_rmdir_rf(p_full_path);
        }
        htd_free(p_full_path);
        p_full_path = NULL;
    }
    closedir(p_dir);
    htd_vfs_file_unlink(p_path);
}

// @brief 打开文件夹
DIR *htd_vfs_dir_open(const char *p_namepath)
{
    int file_id = htd_vfs_file_open(p_namepath, O_RDONLY|O_DIRECTORY);
    if (file_id < 0) {
        return NULL;
    }
    
    DIR * p_dir = (DIR *)htd_malloc(sizeof(DIR));
    if (p_dir == NULL) {
        htd_vfs_file_close(file_id);
    } else {
        htd_memset(p_dir, 0, sizeof(DIR));
        p_dir->fd = file_id;
    }
    return p_dir;
}

// @brief 关闭文件夹
int htd_vfs_dir_close(DIR* p_dir)
{
    if (p_dir) {
        htd_vfs_file_close(p_dir->fd);
        htd_free(p_dir);
        return 0;
    }
    return -1;
}

// @brief 读取文件夹
htd_vfs_dirent_t *htd_vfs_dir_read(DIR *p_dir)
{
    if (p_dir->num) {
        htd_vfs_dirent_t *p_dirent;
        p_dirent = (htd_vfs_dirent_t*)&p_dir->buf[p_dir->cur];
        p_dir->cur += p_dirent->d_reclen;
    }

    int32_t result;
    if (!p_dir->num || p_dir->cur >= p_dir->num) {
        result = htd_vfs_file_getdents(p_dir->fd, (htd_vfs_dirent_t*)p_dir->buf);
        if (result <= 0) {
            return NULL;
        }
        p_dir->num = 1;
        p_dir->cur = 0; 
    }
    return (htd_vfs_dirent_t*)(p_dir->buf + p_dir->cur);
}

// @brief 修改当前路径
int htd_vfs_dir_change(const char *path)
{
    if (path == NULL) {
        htd_print("%s\r\n", htd_vfs_work_dir_get());
        return 0;
    }
    if (htd_strlen(path) > VFS_PATH_MAX) {
        return -1;
    }

    char* p_fullpath = htd_vfs_path_linux_standard(NULL, path);
    if (p_fullpath == NULL) {
        return -ERROR_CODE_NO_SPACE;
    }
    htd_print("workdir=%s, path=%s, fullpath=%s\r\n", htd_vfs_work_dir_get(), path, p_fullpath);

    DIR *p_dir = htd_vfs_dir_open(p_fullpath);
    if (p_dir == NULL) {
        htd_free(p_fullpath);
        htd_print("open dir(%s) failed.\r\n", p_fullpath);
        return -1;
    }
    closedir(p_dir);
    
    htd_vfs_work_dir_set(p_fullpath);
    htd_free(p_fullpath);
    return 0;
}


// @brief 获取文件夹当前位置
long htd_vfs_dir_tell(DIR *p_dir)
{
    if (p_dir == NULL) {
        return -1;
    }
    
    htd_vfs_fd_t *p_fd = vfs_fd_get(p_dir->fd);
    if (p_fd == NULL) {
        return -1;
    }
    return p_fd->pos - p_dir->num + p_dir->cur;
}

// @brief 文件夹下所有文件大小(涉及到递归)
uint32_t htd_vfs_dir_size_get(const char* p_path)
{
    DIR *p_dir;
    char *p_full_path = NULL;
    htd_vfs_dirent_t *p_dirinfo;
    uint32_t dir_size = 0;
    
    p_dir = htd_vfs_dir_open(p_path);
    if (NULL == p_dir) {
        return dir_size;
    }
    while (1) {
        p_dirinfo = htd_vfs_dir_read(p_dir);
        if (NULL == p_dirinfo) {
            break;
        }
        if (htd_strcmp(p_dirinfo->d_name, ".") == 0 || htd_strcmp(p_dirinfo->d_name, "..") == 0) {
            continue;
        }
        if (p_dirinfo->d_type == DT_UNKNOWN) {
            continue;
        }
        p_full_path = htd_vfs_path_linux_standard(p_path, p_dirinfo->d_name);
        if (p_full_path == NULL) {
            continue;
        }
        if (p_dirinfo->d_type != DT_DIR) {
            dir_size += htd_vfs_file_size_get(p_full_path);
        } else {
            dir_size += htd_vfs_dir_size_get(p_full_path);
        }
        htd_free(p_full_path);
        p_full_path = NULL;
    }
    htd_vfs_dir_close(p_dir);
    return dir_size;
}

