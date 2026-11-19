#ifndef __APP_FILE_H__
#define __APP_FILE_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


#ifndef O_DIRECTORY
    #define O_DIRECTORY         0x200000
#endif
#ifndef O_BINARY
    #define O_BINARY            0x8000
#endif
#ifndef O_RDONLY
    #define O_RDONLY            0x0000
#endif
#ifndef O_WRONLY
    #define O_WRONLY            0x0001
#endif
#ifndef O_RDWR
    #define O_RDWR              0x0002
#endif
#ifndef O_APPEND
    #define O_APPEND            0x0008
#endif
#ifndef O_CREAT
    #define O_CREAT             0x0100 
#endif
#ifndef O_TRUNC
    #define O_TRUNC             0x0200
#endif
#ifndef O_EXCL
    #define O_EXCL              0x0400
#endif
#ifndef O_ACCMODE
    #define O_ACCMODE           0x0003
#endif
#ifndef O_NONBLOCK
    #define O_NONBLOCK          0x0004
#endif

typedef void* file_fd_t;

#ifndef ARCH_CPU_64BIT
    typedef int32_t ssize_t;
#else
    typedef int64_t ssize_t;
#endif

// @brief 打开文件
file_fd_t app_file_open(const char* file_path, uint32_t oflag);
// @brief 关闭文件
int app_file_close(file_fd_t handle);
// @brief 读文件数据
ssize_t app_file_read(file_fd_t handle, void *p_buff, size_t buff_size);
// @brief 写文件数据
ssize_t app_file_write(file_fd_t handle, const void *p_buff, size_t buff_len);
// @brief 偏移文件位置
uint32_t app_file_lseek(file_fd_t handle, uint32_t offset);
// @brief 文件信息
uint32_t app_file_size(file_fd_t handle);

#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*__APP_FILE_H__*/

