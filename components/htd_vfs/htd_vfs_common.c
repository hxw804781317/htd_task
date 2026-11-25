// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG              "vfs.common"
#include "htd_vfs_process.h"
#include "htd_vfs_common.h"


// @brief 通过 htd_malloc 拷贝字符串；使用htd_free 进行释放
char *htd_vfs_strdup(const char *s)
{
    size_t len = htd_strlen(s) + 1;
    char *tmp = (char *)htd_malloc(len);
    if (NULL == tmp) {
        return NULL;
    }
    htd_memcpy(tmp, s, len);
    return tmp;
}

// @brief 跳过某个字符
char* htd_vfs_skip_ch(const char* p_data, char ch)
{
    while (*p_data == ch) {
        p_data++;
    }
    return (char*)p_data;
}

// @brief 路径往前一级
char* htd_vfs_path_up_one(const char* p_path, uint32_t path_len)
{
    const char* p_end = p_path + path_len - 1;
    if (*p_end == '/') {
        p_end--;
    }
    while (p_end >= p_path) {
        if (p_end[0] == '/') {
            return (char*)p_end;
        }
        p_end--;
    }
    return NULL;
}

// @brief 路径下一级
char* htd_vfs_path_down_one(const char* p_path)
{
    p_path = htd_vfs_skip_ch(p_path, '/');
    while (*p_path != '/' && *p_path) {
        p_path++;
    }
    return (char*)p_path;
}

// @brief 判断是否为可视字符
static inline bool vfs_char_is_visible(char ch)
{
    return (ch > 32 && ch != 127);
}

// @brief 路径规范化
// @param p_dirpath:
// @param p_filename
// @return 返回规范化后的路径 standard
char *htd_vfs_path_linux_standard(const char *p_dirpath, const char *p_filename)
{
    ASSERT(p_filename != NULL);
    if (p_dirpath == NULL) {
        p_dirpath = htd_vfs_work_dir_get();
    }

    char *p_fullpath;
    uint32_t fullpath_len = 0;
    uint32_t filename_len = htd_strlen(p_filename) + 1;
    if (p_filename[0] != '/')  {
        fullpath_len = htd_strlen(p_dirpath) + 1 + filename_len;
        p_fullpath = (char *)htd_malloc(fullpath_len);
        if (p_fullpath == NULL) {
            return NULL;
        }
        htd_snprintf(p_fullpath, fullpath_len, "%s/%s", p_dirpath, p_filename);
    } else {
        fullpath_len = filename_len;
        p_fullpath = htd_malloc(fullpath_len); 
        if (p_fullpath == NULL) {
            return NULL;
        }
        htd_memcpy(p_fullpath, p_filename, fullpath_len);
    }

    // 去掉所有不可视字符, 并将所有 '\' 转换为 '/'
    char *p_src = p_fullpath;
    char *p_dst = p_fullpath;
    while (*p_src) {
        if (vfs_char_is_visible(*p_src)) {
            if (*p_src == '\\') {
                *p_src = '/';
            }
            *p_dst++ = *p_src;
        }
        p_src++;
    }
    *p_dst = 0;
    
    p_dst = p_fullpath;
    p_src = p_fullpath;
    while (1) {
        if (p_src[0] == '.')  {
            if (0 == p_src[1]) {              // 处理：'.'
                break;
            } 
            if (p_src[1] == '/') {            // 处理 "./"
                p_src = htd_vfs_skip_ch(p_src + 1, '/');
                continue;
            } 
            if (p_src[1] == '.') {
                if (0 == p_src[2]) {          // 处理 ".."
                    p_src += 2;
                    p_dst = htd_vfs_path_up_one(p_fullpath, p_dst-p_fullpath);
                    if (p_dst == NULL) {
                        htd_free(p_fullpath);
                        return NULL;
                    }
                    p_dst++;
                    break;
                } else if (p_src[2] == '/') { // 处理"../"
                    p_src = htd_vfs_skip_ch(p_src + 2, '/');
                    p_dst = htd_vfs_path_up_one(p_fullpath, p_dst-p_fullpath);
                    if (p_dst == NULL) {
                        htd_free(p_fullpath);
                        return NULL;
                    }
                    p_dst++;
                    continue;
                }
            }
        }

        // 拷贝数据
        while (*p_src && *p_src != '/') {
            *p_dst++ = *p_src++;
        }
        if (*p_src == '/') {
            *p_dst++ = '/';
            p_src = htd_vfs_skip_ch(p_src, '/');
        }
        if (*p_src == 0) {
            break;
        } 
    }
    *p_dst = 0;

    // 去除尾部的斜杆
    p_dst--;
    if (p_dst != p_fullpath && *p_dst == '/') {
        *p_dst = 0;
    }
    if (0 == p_fullpath[0]) {
        p_fullpath[0] = '/';
        p_fullpath[1] = 0;
    }
    return p_fullpath;
}

// @brief 获取子目录
const char *htd_vfs_subdir(const char *p_dir, const char *filename)
{
    uint32_t dir_len = htd_strlen(p_dir);
    if (dir_len == htd_strlen(filename)) {
        return NULL;
    }

    const char *dir = filename + dir_len;
    if ((*dir != '/') && (dir != filename)) {
        dir--;
    }
    return dir;
}

// @brief 获取文件名
const char *htd_vfs_file_basename_get(const char *path)
{
    char *ptr = (char *)strrchr(path, '/');
    if (ptr == NULL) {
        return path;
    }
    return ++ptr;
}

