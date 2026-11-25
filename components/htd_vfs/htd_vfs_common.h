// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_COMMON_H_
#define _HTD_VFS_COMMON_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// 控制挂载的文件系统数量
#ifndef VFS_FILESYSTEMS_MAX
    #define VFS_FILESYSTEMS_MAX     8
#endif
#ifndef HTD_VFS_FD_MAX
    #define HTD_VFS_FD_MAX          32
#endif
#ifndef VFS_FD_OFFSET
    #define VFS_FD_OFFSET           3
#endif
#ifndef VFS_PATH_MAX
    #define VFS_PATH_MAX            256
#endif
#ifndef __STATIC_INLINE
    #define __STATIC_INLINE         static inline
#endif
#ifndef ARRAY_COUNT
    #define ARRAY_COUNT(X)          (sizeof(X) / sizeof(X[0]))
#endif
#ifndef ALIGN_DOWN
    #define ALIGN_DOWN(n, align)    ((n) & ~((align) - 1))
#endif 
#ifndef ALIGN_UP
    #define ALIGN_UP(n, align)      (((n) + (align) - 1) & ~((align) - 1))
#endif 

// 文件类型
#define FT_REGULAR               0   /* regular file */
#define FT_SOCKET                1   /* socket file  */
#define FT_DIRECTORY             2   /* directory    */
#define FT_USER                  3   /* user defined */
#define FT_DEVICE                4   /* device */
// 文件flag
#define HTD_F_OPEN              0x01000000
#define HTD_F_EOF               0x04000000
#define HTD_F_ERR               0x08000000


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

#define F_DUPFD  0
#define F_GETFD  1
#define F_SETFD  2
#define F_GETFL  3
#define F_SETFL  4
#define F_SETOWN 8
#define F_GETOWN 9
#define F_SETSIG 10
#define F_GETSIG 11
#define F_GETLK 12
#define F_SETLK 13
#define F_SETLKW 14
#define F_SETOWN_EX 15
#define F_GETOWN_EX 16
#define F_GETOWNER_UIDS 17

#define VFS_F_TRUNCATE              0xFA5A0000U
#define VFS_F_RESERVE_CONT_SPACE    0xFA5A0001U
#define VFS_F_PHY_ADDR_GET          0xFA5A0002U

#ifndef S_IFMT
    #define S_IFMT               00170000
#endif
#ifndef S_IFSOCK
    #define S_IFSOCK             0140000
#endif
#ifndef S_IFLNK
    #define S_IFLNK              0120000
#endif
#ifndef S_IFREG
    #define S_IFREG              0100000
#endif
#ifndef S_IFBLK
    #define S_IFBLK              0060000
#endif
#ifndef S_IFDIR
    #define S_IFDIR              0040000
#endif
#ifndef S_IFCHR
    #define S_IFCHR              0020000
#endif
#ifndef S_IFIFO
    #define S_IFIFO              0010000
#endif
#ifndef S_ISUID
    #define S_ISUID              0004000
#endif
#ifndef S_ISGID
    #define S_ISGID              0002000
#endif
#ifndef S_ISVTX
    #define S_ISVTX              0001000
#endif
#ifndef S_ISLNK
    #define S_ISLNK(m)           (((m) & S_IFMT) == S_IFLNK)
#endif
#ifndef S_ISREG
    #define S_ISREG(m)           (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
    #define S_ISDIR(m)           (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISCHR
    #define S_ISCHR(m)           (((m) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISBLK
    #define S_ISBLK(m)           (((m) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISFIFO
    #define S_ISFIFO(m)          (((m) & S_IFMT) == S_IFIFO)
#endif
#ifndef S_ISSOCK
    #define S_ISSOCK(m)          (((m) & S_IFMT) == S_IFSOCK)
#endif
#ifndef S_IRWXU
    #define S_IRWXU              00700
#endif
#ifndef S_IRUSR
    #define S_IRUSR              00400
#endif
#ifndef S_IWUSR
    #define S_IWUSR              00200
#endif
#ifndef S_IXUSR
    #define S_IXUSR              00100
#endif
#ifndef S_IRWXG
    #define S_IRWXG              00070
#endif
#ifndef S_IRGRP
    #define S_IRGRP              00040
#endif
#ifndef S_IWGRP
    #define S_IWGRP              00020
#endif
#ifndef S_IXGRP
    #define S_IXGRP              00010
#endif
#ifndef S_IRWXO
    #define S_IRWXO              00007
#endif
#ifndef S_IROTH
    #define S_IROTH              00004
#endif
#ifndef S_IWOTH
    #define S_IWOTH              00002
#endif
#ifndef S_IXOTH
    #define S_IXOTH              00001
#endif






//////////////////////////////////////////////////////////////////////
#if defined(__CC_ARM) || defined(__CLANG_ARM) || defined(__IAR_SYSTEMS_ICC__)
    typedef signed long htd_vfs_off_t;
#elif defined(__MINGW32__) || defined(_WIN32)
    typedef signed long htd_vfs_off_t;
#elif defined(ARCH_CPU_64BIT)
    typedef int64_t htd_vfs_off_t;
#else
    typedef int32_t htd_vfs_off_t;
#endif
#ifndef ARCH_CPU_64BIT
    typedef int32_t ssize_t;
#else
    typedef int64_t ssize_t;
#endif

typedef uint32_t vfs_addr_t;
typedef struct {
    size_t f_bsize;             /* block size */
    size_t f_blocks;            /* total data blocks in file system */
    size_t f_bfree;             /* free blocks in file system */
} htd_vfs_statfs_t;

struct dirent {
    uint8_t d_type;             // 文件类型 @DT_DIR, @DT_REG
    uint8_t d_namlen;           /* The length of the not including the terminating null file name */
    uint16_t d_reclen;          /* length of this record */
    char d_name[256];           /* The null-terminated file name */
};
typedef struct dirent htd_vfs_dirent_t;

typedef struct {
    int fd;                     /* directory file */
    char buf[sizeof(htd_vfs_dirent_t)];
    int num;
    int cur;
} vfs_dir_t;
typedef vfs_dir_t DIR;
typedef uint32_t vfs_mode_t;


typedef struct {
    uint16_t st_ino;
    uint16_t st_mode;
    uint16_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint32_t st_size;
    uint32_t st_atime;
    uint32_t st_mtime;
} htd_vfs_stat_t;


//////////////////////////////////////////////////////////////////////
#define VFS_FD_MAGIC     0xfdfd

#ifndef SEEK_SET
    #define SEEK_SET        0
#endif
#ifndef SEEK_CUR
    #define SEEK_CUR        1
#endif
#ifndef SEEK_END
    #define SEEK_END        2
#endif

#ifndef DT_DIR
    #define DT_DIR          2
#endif
#ifndef DT_REG
    #define DT_REG          1
#endif
#ifndef DT_UNKNOWN
    #define DT_UNKNOWN      0
#endif


// 文件系统类型
#define HTD_VFS_TYPE_NONE           0
#define HTD_VFS_TYPE_RAMFS          1
#define HTD_VFS_TYPE_ROMFS          2
#define HTD_VFS_TYPE_ELM_FATFS      3
#define HTD_VFS_TYPE_SPIFFS         4
#define HTD_VFS_TYPE_LITTLE_FS      5
#define HTD_VFS_TYPE_UFFS           6
#define HTD_VFS_TYPE_WIN32          7
typedef uint8_t htd_vfs_type_t;

// 文件系统
typedef struct vfs_fd_s htd_vfs_fd_t;

// 文件系统接口
typedef struct htd_vfs_s htd_vfs_t;
typedef struct htd_vfs_api_s htd_vfs_api_t;
struct htd_vfs_api_s {
    uint32_t magic;
    htd_vfs_type_t type;
    int (*f_open)(htd_vfs_fd_t *p_fd);
    int (*f_close)(htd_vfs_fd_t *p_fd);
    int (*f_ioctl)(htd_vfs_fd_t *p_fd, int cmd, void *args);
    ssize_t (*f_read)(htd_vfs_fd_t *p_fd, void *p_buff, size_t count);
    ssize_t (*f_write)(htd_vfs_fd_t *p_fd, const void *p_buff, size_t count);
    htd_vfs_off_t (*f_lseek)(htd_vfs_fd_t *p_fd, htd_vfs_off_t offset);
    int (*f_flush)(htd_vfs_fd_t *p_fd);
    int (*f_getdents)(htd_vfs_fd_t *p_fd, htd_vfs_dirent_t *p_entry);
    
    int (*f_unlink)(htd_vfs_t *p_vfs, const char *p_pathname);
    int (*f_stat)(htd_vfs_t *p_vfs, const char *p_filename, htd_vfs_stat_t *p_info);
    int (*f_rename)(htd_vfs_t *p_vfs, const char *p_oldpath, const char *p_newpath);
    int (*fs_mount)(htd_vfs_t *p_vfs, const void *p_data);
    int (*fs_unmount)(htd_vfs_t *p_vfs);
    int (*fs_mk)(htd_vfs_t *p_vfs, const void* p_param);
    int (*fs_stat)(htd_vfs_t *p_vfs, htd_vfs_statfs_t *buf);
};

// 文件系统接口
struct htd_vfs_s {
    htd_slist_node_t node;
    char* p_path;
    void* p_fs_dev;

    htd_vfs_type_t type;
    int (*f_open)(htd_vfs_fd_t *p_fd);
    int (*f_close)(htd_vfs_fd_t *p_fd);
    int (*f_ioctl)(htd_vfs_fd_t *p_fd, int cmd, void *args);
    ssize_t (*f_read)(htd_vfs_fd_t *p_fd, void *p_buff, size_t count);
    ssize_t (*f_write)(htd_vfs_fd_t *p_fd, const void *p_buff, size_t count);
    htd_vfs_off_t (*f_lseek)(htd_vfs_fd_t *p_fd, htd_vfs_off_t offset);
    int (*f_flush)(htd_vfs_fd_t *p_fd);
    int (*f_getdents)(htd_vfs_fd_t *p_fd, htd_vfs_dirent_t *p_entry);
    
    int (*f_unlink)(htd_vfs_t *p_vfs, const char *p_pathname);
    int (*f_stat)(htd_vfs_t *p_vfs, const char *p_filename, htd_vfs_stat_t *p_info);
    int (*f_rename)(htd_vfs_t *p_vfs, const char *p_oldpath, const char *p_newpath);
    int (*fs_mount)(htd_vfs_t *p_vfs, const void *p_data);
    int (*fs_unmount)(htd_vfs_t *p_vfs);
    int (*fs_mk)(htd_vfs_t *p_vfs, const void* p_param);
    int (*fs_stat)(htd_vfs_t *p_vfs, htd_vfs_statfs_t *buf);
} ;

struct vfs_fd_s {
    uint16_t magic;
    uint16_t type; 
    int fd;
    int ref_count; 
    uint32_t flags; 
    htd_vfs_off_t size;
    htd_vfs_off_t pos; 
    htd_vfs_t *p_vfs;
    char *path;
    void *data; 
};

// 注册
#define HTD_VFS_MAGIC               0x3a098765
#define HTD_VFS_API_SECTION_NAME    htd_vfs_api
#define HTD_VFS_API_REGISTER(_type, _fopen, _fclose, _fioctl, _fread, _fwrite, _flseek, _fflush, _fgetdents, _funlink, _fstat, _frename, _fs_mount, _fs_unmount, _fs_mk, _fs_stat)   \
    HTD_SECTION_ITEM_REGISTER(HTD_VFS_API_SECTION_NAME, static const htd_vfs_api_t CONCAT_2(m_vfs_api_, _type)) = {\
        .magic = HTD_VFS_MAGIC,             \
        .type = _type,                      \
        .f_open = _fopen,                   \
        .f_close = _fclose,                 \
        .f_ioctl = _fioctl,                 \
        .f_read = _fread,                   \
        .f_write = _fwrite,                 \
        .f_lseek = _flseek,                 \
        .f_flush = _fflush,                 \
        .f_getdents = _fgetdents,           \
        .f_unlink = _funlink,               \
        .f_stat = _fstat,                   \
        .f_rename = _frename,               \
        .fs_mount = _fs_mount,              \
        .fs_unmount = _fs_unmount,          \
        .fs_mk = _fs_mk,                    \
        .fs_stat = _fs_stat,                \
    };


//////////////////////////////////////////////////////////////////////
// @brief 通过 htd_malloc 拷贝字符串；使用htd_free 进行释放
char *htd_vfs_strdup(const char *s);
// @brief 路径规范化
// @param p_dirpath:
// @param p_filename
// @return 返回规范化后的路径
char *htd_vfs_path_linux_standard(const char *p_dirpath, const char *p_filename);
// @brief 
const char *htd_vfs_subdir(const char *directory, const char *filename);
// @brief 获取文件名
const char *htd_vfs_file_basename_get(const char *path);
// @brief 跳过某个字符
char* htd_vfs_skip_ch(const char* p_data, char ch);
// @brief 路径往前一级
char* htd_vfs_path_up_one(const char* p_path, uint32_t path_len);
// @brief 路径下一级
char* htd_vfs_path_down_one(const char* p_path);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_COMMON_H_*/

