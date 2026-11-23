// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_PROCESS_H_
#define _HTD_VFS_PROCESS_H_

#include "htd_vfs_port.h"
#include "htd_vfs_file.h"
#include "htd_vfs_posix.h"

#ifdef HTD_VFS_USING_RAMFS
    #include "htd_vfs_ramfs.h"
#endif
#ifdef HTD_VFS_USING_ROMFS
    #include "htd_vfs_romfs.h"
#endif
#ifdef HTD_VFS_USING_ELM_FATFS
    #include "htd_vfs_fatfs.h"
#endif
#ifdef HTD_VFS_USING_SPIFFS
    #include "htd_vfs_spiffs.h"
#endif
#ifdef HTD_VFS_USING_LITTLE_FS
    #include "htd_vfs_lfs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief 加锁
void htd_vfs_lock(void);
// @brief 解锁
void htd_vfs_unlock(void);
// @brief 获取工作路径
const char* htd_vfs_work_dir_get(void);
// @brief 设置工作路径
void htd_vfs_work_dir_set(const char* p_path);
// @brief 初始化
int htd_vfs_init(void);


// @brief 挂载文件系统
// @param fs_type 文件系统类型
// @param p_path 挂载的路径
// @param data 用户数据
// @param is_force true挂载失败时会直接格式化，再重新挂载
// @return 成功返回0，失败返回非0数值
int htd_vfs_mount(htd_vfs_type_t fs_type, const char *p_path, const void *fs_dev, bool is_force);
// @brief 卸载文件系统
// @param p_path 挂载的路径
// @return 成功返回0，失败返回小于0
int htd_vfs_unmount(const char *p_path);


// @brief 迭代查询文件系统
htd_vfs_t *htd_vfs_query(htd_vfs_t *p_vfs);
// @brief 通过挂载的文件路径进行查找
htd_vfs_t *htd_vfs_find(const char *p_path);



// @brief 创建文件系统(该文件系统必须已经挂载)
// @param p_fs_name 文件系统名字
// @return 成功返回0，失败返回负值
int htd_vfs_mkfs(const char *p_fs_name, void* p_param);
// @brief 获取文件系统信息
int htd_vfs_statfs(const char *p_path, htd_vfs_statfs_t *p_statfs);
// @brief 显示文件系统信息
int htd_vfs_df(const char *path);
// @brief 获取文件系统剩余空间大小
uint32_t htd_vfs_free_space_size_get(const char *p_path);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_PROCESS_H_*/

