// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_POSIX_H_
#define _HTD_VFS_POSIX_H_

#include "htd_vfs_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#if defined(HTD_VFS_USING_POSIX)
// @brief 打开函数，成功返回文件描述符，失败返回-1
int open(const char *filepath, int flags, ...);
// @brief 创建函数，成功返回文件描述符，失败返回-1
int creat(const char *path, vfs_mode_t mode);
// @brief 关闭文件，成功返回0，失败返回对应的错误码
int close(int fd);
// @brief 读文件，成功返回读取的数据长度，失败返回错误码，返回0代表文件已经没有数据了
ssize_t read(int fd, void *buf, size_t len);
// @brief 写文件，成功返回写入的数据长度，失败返回错误码
ssize_t write(int fd, const void *buf, size_t len);
// @brief 偏移文件位置，成功返回当前文件的偏移量，失败返回对应的错误码
htd_vfs_off_t lseek(int fd, htd_vfs_off_t offset, int whence);


#ifndef _WIN32
// @brief 重命名文件，成功返回0，失败返回对应的错误码
int rename(const char *old_file, const char *new_file);
#endif


// @brief 删除文件，成功返回0，失败返回对应的错误码
int unlink(const char *pathname);
// @brief 获取文件信息，成功返回0，失败返回对应的错误码
int stat(const char *filepath, htd_vfs_stat_t *p_info);
// @brief 获取文件信息，成功返回0，失败返回对应的错误码
int fstat(int fildes, htd_vfs_stat_t *p_info);
// @brief 同步文件数据，成功返回0，失败返回对应的错误码
int fsync(int fildes);
// @brief 文件控制
int fcntl(int fildes, int cmd, ...);
// @brief 文件控制
int ioctl(int fildes, int cmd, ...);
// @brief 文件截断，成功返回0，失败返回对应的错误码
int ftruncate(int fd, htd_vfs_off_t length);
// @brief 获取文件信息
int statfs(const char *path, htd_vfs_statfs_t *buf);


// @brief 创建文件夹，成功返回0，失败返回对应的错误码
int mkdir(const char *path, vfs_mode_t mode);
// @brief 递归创建文件夹，类似于 mkdir -pp, 成功返回0，失败返回对应的错误码
bool mkdir_pp(const char* p_path);
// @brief 删除文件夹
int rmdir(const char *pathname);
// @brief 递归杉树文件夹
int rmdir_rf(const char *pathname);


// @brief 打开文件夹
DIR *opendir(const char *name);
// @brief 读取文件夹
htd_vfs_dirent_t *readdir(DIR *d);
// @brief 获取目录流 DIR* 当前的读取位置标记
long telldir(DIR *d);
// @brief 设置目录流位置
void seekdir(DIR *d, htd_vfs_off_t offset);
// @brief 将目录流位置重置回目录开头
void rewinddir(DIR *d);
// @brief 关闭文件夹
int closedir(DIR *d);


// @brief 修改当前的工作目录，等同命令行 cd
int chdir(const char *path);
// @brief 获取当前的工作目录，等同命令行pwd
char *getcwd(char *buf, size_t size);
// @brief 检查文件是否存在
int access(const char *path, int amode);


#endif /*HTD_FS_USING_POSIX*/
#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_POSIX_H_*/

