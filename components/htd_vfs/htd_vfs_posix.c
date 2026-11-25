// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG              "vfs.posix"
#include "htd_vfs_port.h"
#include "htd_vfs_process.h"
#include "htd_vfs_file.h"
#include "htd_vfs_posix.h"


#if defined(HTD_VFS_USING_POSIX)
// @brief 打开函数，成功返回文件描述符，失败返回-1
int open(const char *filepath, int flags, ...)
{
    return htd_vfs_file_open(filepath, flags);
}

// @brief 创建函数，成功返回文件描述符，失败返回-1
int creat(const char *path, vfs_mode_t mode)
{
    return open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

// @brief 关闭文件，成功返回0，失败返回对应的错误码
int close(int fd)
{
    return htd_vfs_file_close(fd);;
}

// @brief 读文件，成功返回读取的数据长度，失败返回错误码，返回0代表文件已经没有数据了
ssize_t read(int fd, void *buf, size_t len)
{
    return htd_vfs_file_read(fd, buf, len);
}

// @brief 写文件，成功返回写入的数据长度，失败返回错误码
ssize_t write(int fd, const void *buf, size_t len)
{
    return htd_vfs_file_write(fd, buf, len);
}

// @brief 偏移文件位置，成功返回当前文件的偏移量，失败返回对应的错误码
htd_vfs_off_t lseek(int fd, htd_vfs_off_t offset, int whence)
{
    return htd_vfs_file_lseek(fd, offset, whence);
}

#ifndef _WIN32
// @brief 重命名文件，成功返回0，失败返回对应的错误码
int rename(const char *old_file, const char *new_file)
{
    return htd_vfs_rename(old_file, new_file);
}
#endif

// @brief 删除文件，成功返回0，失败返回对应的错误码
int unlink(const char *pathname)
{
    return htd_vfs_file_unlink(pathname);
}

// @brief 获取文件信息，成功返回0，失败返回对应的错误码
int stat(const char *filepath, htd_vfs_stat_t *p_info)
{
    return htd_vfs_file_stat(filepath, p_info);
}

// @brief 获取文件信息，成功返回0，失败返回对应的错误码
int fstat(int fildes, htd_vfs_stat_t *p_info)
{
    return htd_vfs_file_fstat(fildes, p_info);
}

// @brief 同步文件数据，成功返回0，失败返回对应的错误码
int fsync(int fildes)
{
    return htd_vfs_file_fsync(fildes);
}

// @brief 文件控制
int fcntl(int fildes, int cmd, ...)
{
    void *arg;
    va_list ap;
    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);
    return htd_vfs_file_fcntl(fildes, cmd, arg);
}

// @brief 文件控制
int ioctl(int fildes, int cmd, ...)
{
    void *arg;
    va_list ap;
    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);
    return htd_vfs_file_fcntl(fildes, cmd, arg);
}

// @brief 文件截断，成功返回0，失败返回对应的错误码
int ftruncate(int fd, htd_vfs_off_t length)
{
    return htd_vfs_file_ftruncate(fd, length);
}

// @brief 获取文件信息
int statfs(const char *path, htd_vfs_statfs_t *buf)
{
    int result = htd_vfs_statfs(path, buf);
    if (result < 0) {
        return -ERROR_CODE_INVALID_STATE;
    }
    return result;
}

// @brief 创建文件夹，成功返回0，失败返回对应的错误码
int mkdir(const char *path, vfs_mode_t mode)
{
    return htd_vfs_mkdir(path);
}

// @brief 递归创建文件夹，类似于 mkdir -pp, 成功返回0，失败返回对应的错误码
bool mkdir_pp(const char* p_path)
{
    return htd_vfs_mkdir_pp(p_path);
}

// @brief 删除文件夹
int rmdir(const char *pathname)
{
    return htd_vfs_file_unlink(pathname);
}

// @brief 递归杉树文件夹
int rmdir_rf(const char *pathname)
{
    htd_vfs_rmdir_rf(pathname);
    return 0;
}

// @brief 打开文件夹
DIR *opendir(const char *name)
{
    return htd_vfs_dir_open(name);
}

// @brief 读取文件夹
htd_vfs_dirent_t *readdir(DIR *d)
{
    return htd_vfs_dir_read(d);
}

// @brief 获取目录流 DIR* 当前的读取位置标记
long telldir(DIR *d)
{
    return htd_vfs_dir_tell(d);
}

// @brief 设置目录流位置
void seekdir(DIR *d, htd_vfs_off_t offset)
{
    if (htd_vfs_file_lseek(d->fd, offset, SEEK_SET) >= 0) {
        d->num = d->cur = 0;
    }
}

// @brief 将目录流位置重置回目录开头
void rewinddir(DIR *d)
{
    if (htd_vfs_file_lseek(d->fd, 0, SEEK_SET) >= 0) {
        d->num = d->cur = 0;
    }
}

// @brief 关闭文件夹
int closedir(DIR *d)
{
    return htd_vfs_dir_close(d);
}

// @brief 修改当前的工作目录，等同命令行 cd
int chdir(const char *path)
{
    return htd_vfs_dir_change(path);
}

// @brief 获取当前的工作目录，等同命令行pwd
char *getcwd(char *buf, size_t size)
{
    htd_vfs_lock();
    htd_strncpy(buf, htd_vfs_work_dir_get(), size);
    htd_vfs_unlock();
    return buf;
}

// @brief 检查文件是否存在
int access(const char *path, int amode)
{
    htd_vfs_stat_t fileinfo;
    if (stat(path, &fileinfo) < 0) {
        return -1;
    }
    return 0;
}

#endif /*HTD_FS_USING_POSIX*/

