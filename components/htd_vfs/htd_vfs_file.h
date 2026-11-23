// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_FILE_H_
#define _HTD_VFS_FILE_H_

#include "htd_vfs_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// @brief 判断文件是否已经打开
int htd_vfs_file_is_open(const char *p_pathname);
// @brief 打开文件(对应C标准函数fopen)
// @return 成功返回对应的文件描述符，失败返回NULL
htd_vfs_fd_t* htd_vfs_fopen(const char *p_path, const char* mode);
// @brief 关闭文件(对应C标准函数fclose)
// @return 成功返回0，失败返回小于0的错误码
int htd_vfs_fclose(htd_vfs_fd_t *p_fd);
// @brief 读文件数据(对应C标准函数fread)
// @return 返回读取到的数据长度，失败返回小于0的错误码
ssize_t htd_vfs_fread(htd_vfs_fd_t *p_fd, void *p_buff, size_t buff_size);
// @brief 写文件数据(对应C标准函数fwrite)
// @return 成功返回写入的数据长度，失败返回小于0的错误码
ssize_t htd_vfs_fwrite(htd_vfs_fd_t *p_fd, const void *p_buff, size_t buff_len);
// @brief 刷新文件缓冲数据(对应C标准函数fflush)
int htd_vfs_fflush(htd_vfs_fd_t *p_fd);
// @brief 重定位文件位置(对应C标准函数fseek)
htd_vfs_off_t htd_vfs_fseek(htd_vfs_fd_t *p_fd, htd_vfs_off_t offset, int whence);
// @brief 返回当前文件的位置(对应C标准函数ftell)
htd_vfs_off_t htd_vfs_ftell(htd_vfs_fd_t* p_fd);
// @brief 判断是否已经达到文件末尾(对应C标准函数feof)
bool htd_vfs_feof(htd_vfs_fd_t* p_fd);
// @brief 文件大小
htd_vfs_off_t htd_vfs_fsize(htd_vfs_fd_t* p_fd);
// @brief 删除文件
int htd_vfs_remove(const char *path);
// @brief 重命名(对应C标准函数rename)
int htd_vfs_rename(const char *p_oldpath, const char *p_newpath);
// @brief 把文件指针移到开始处(对应C标准函数rewind)
void htd_vfs_rewind(htd_vfs_fd_t* p_fd);





//////////////////////////////////////////////////////////////////////
// @brief 打开文件(对应linux函数open)
int htd_vfs_file_open(const char *p_path, int flags);
// @brief 关闭文件(对应 linux 函数 close)
int htd_vfs_file_close(int file_id);
// @brief 读文件数据(对应 linux 函数 read)
ssize_t htd_vfs_file_read(int file_id, void *p_buff, size_t buff_size);
// @brief 写文件数据(对应 linux 函数 write)
ssize_t htd_vfs_file_write(int file_id, const void *p_buff, size_t buff_len);
// @brief 文件io控制(对应 linux 函数 ioctl)
// @return 成功返回0，失败返回小于0的错误码
int htd_vfs_file_ioctl(int file_id, int cmd, void *args);
// @brief 文件控制(对应 linux 函数 fcntl)
int htd_vfs_file_fcntl(int file_id, int cmd, ...);
// @brief 获取目录信息
int htd_vfs_file_getdents(int file_id, htd_vfs_dirent_t *p_dir);
// @brief 删除文件(对应 linux 函数 unlink)
int htd_vfs_file_unlink(const char *path);
// @brief 刷新文件缓冲数据(对应 linux 函数 fsync)
int htd_vfs_file_fsync(int file_id);
// @brief 重定位文件位置(对应 linux 函数 lseek)
htd_vfs_off_t htd_vfs_file_lseek(int file_id, htd_vfs_off_t offset, int whence);
// @brief 获取文件信息
int htd_vfs_file_stat(const char *path, htd_vfs_stat_t *p_info);
// @brief 获取文件信息(对应 linux 函数 fstat)
int htd_vfs_file_fstat(int file_id, htd_vfs_stat_t *p_info);
// @brief 重置文件大小
int htd_vfs_file_ftruncate(int file_id, htd_vfs_off_t length);


// @brief 显示文件列表
void htd_vfs_ls(const char *p_pathname);
// @brief 显示文件内容
void htd_vfs_file_cat(const char *p_filename);
// @brief 获取文件大小
uint32_t htd_vfs_file_size_get(const char* filepath);
// @brief 拷贝
void htd_vfs_copy(const char *src, const char *dst);


// @brief 创建目录
int htd_vfs_mkdir(const char *p_path);
// @brief 创建对应的文件路径
bool htd_vfs_mkdir_pp(const char* p_path);
// @brief 删除文件夹以及文件夹里面所有文件
void htd_vfs_rmdir_rf(const char* p_path);
// @brief 打开文件夹
DIR *htd_vfs_dir_open(const char *p_namepath);
// @brief 关闭文件夹
int htd_vfs_dir_close(DIR* p_dir);
// @brief 读取文件夹
htd_vfs_dirent_t *htd_vfs_dir_read(DIR *p_dir);
// @brief 修改当前路径
int htd_vfs_dir_change(const char *path);
// @brief 获取文件夹当前位置
long htd_vfs_dir_tell(DIR *p_dir);
// @brief 文件夹下所有文件大小(涉及到递归)
uint32_t htd_vfs_dir_size_get(const char* p_path);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_FILE_H_*/

