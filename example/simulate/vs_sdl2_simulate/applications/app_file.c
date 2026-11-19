/*
 * Copyright (c) 2025 Daven <804781317@qq.com>
 * All rights reserved
 */
#define DEBUG_LVL 4
#define DEBUG_TAG              "file"
#include "htd_task.h"
#include "app_file.h"
#include <io.h>
#include <WinError.h>
#include <windows.h>


// @brief 错误码结果
static int win32_errcode_result_get(DWORD res)
{
    /* There are so many error codes in windows, you'd better google for details.
     * google  "System Error Codes (Windows)"
     * http://msdn.microsoft.com/ZH-CN/library/windows/desktop/ms681381(v=vs.85).aspx
     */
    struct _errcode_map {
        uint16_t dfserr;
        uint16_t win32err;
    };
    static const struct _errcode_map errcode_table[] = {
        {ERROR_CODE_NOT_SUPPORTED, ERROR_FILE_NOT_FOUND },
        {ERROR_CODE_NOT_SUPPORTED, ERROR_PATH_NOT_FOUND },
        {EEXIST, ERROR_FILE_EXISTS },
        {EEXIST, ERROR_ALREADY_EXISTS },
        {ENOTEMPTY, ERROR_DIR_NOT_EMPTY },
        {ERROR_CODE_BUSY, ERROR_PATH_BUSY },
        {ERROR_CODE_INVALID_PARAM, ERROR_ACCESS_DENIED },
    };
    print_error("error. res=%d\n", res);
    ASSERT(0);
    for (int i=0; i<ARRAY_COUNT(errcode_table); i++) {
        if (errcode_table[i].win32err == (res & 0xFFFF)) {
            return (0 - (int)errcode_table[i].dfserr);
        }
    }
    print_error("win32 error not supported: res=%d\n", res);
    return -1;
}

// @brief 拷贝目录路径
static char *win32_dirpath_dup(const char *p_src)
{
    uint32_t len = 16;
    if (p_src) {
        len += htd_strlen(p_src) + 1;
    }

    char* p_path = htd_malloc(len);
    if (p_path == NULL) {
        return NULL;
    }
    len = htd_snprintf(p_path, len, "%s", p_src);
    
    char* p_start = p_path;
    while (*p_path) {
        if (*p_path == '/') {
            *p_path = '\\';
        }
        p_path++;
    }
    return p_start;
}

// @brief 打开文件
file_fd_t app_file_open(const char* file_path, uint32_t oflag)
{
    long create_type = OPEN_EXISTING;   // 文件必须已经存在
    if (oflag & O_CREAT) {              // 如文件不存在则创建它
        create_type = OPEN_ALWAYS;
    } else if (oflag & O_TRUNC) {              // 现有文件缩短为零长度
        create_type = TRUNCATE_EXISTING;
    }

    char* p_filepath = win32_dirpath_dup(file_path);
    HANDLE file_handle = CreateFile(p_filepath, GENERIC_READ|GENERIC_WRITE, 0, NULL, create_type, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        print_error("create file failed. filepath=%s\n", p_filepath);
        htd_free(p_filepath);
        win32_errcode_result_get(GetLastError());
        return NULL;
    }
    htd_free(p_filepath);
    
    uint32_t pos = 0;
    if (oflag & O_APPEND){
        LARGE_INTEGER fileSize;
        fileSize.QuadPart = 0;
        GetFileSizeEx(file_handle, &fileSize);
        pos = (uint32_t)fileSize.QuadPart;
    } 
    SetFilePointer(file_handle, pos, NULL, FILE_BEGIN);
    return (file_fd_t)file_handle;
}

// @brief 关闭文件
int app_file_close(file_fd_t handle)
{
    HANDLE file_handle = (HANDLE)handle;
    if (file_handle) {
        CloseHandle(file_handle);
    }
    return 0;
}

// @brief 读文件数据
ssize_t app_file_read(file_fd_t handle, void *p_buff, size_t buff_size)
{
    DWORD read_len = 0;
    HANDLE file_handle = (HANDLE)handle;
    if (!ReadFile(file_handle, p_buff, buff_size, &read_len, NULL)) {
        print_error("buff_size=%d, read_len=%u\n", buff_size, read_len);
        ASSERT(0);
        return win32_errcode_result_get(GetLastError());
    }
    return read_len;
}

// @brief 写文件数据
ssize_t app_file_write(file_fd_t handle, const void *p_buff, size_t buff_len)
{
    DWORD write_len = 0;
    HANDLE file_handle = (HANDLE)handle;
    if (!WriteFile(file_handle, p_buff, buff_len, &write_len, NULL)) {
        ASSERT(0);
        return win32_errcode_result_get(GetLastError());
    }
    return write_len;
}

// @brief 偏移文件位置
uint32_t app_file_lseek(file_fd_t handle, uint32_t offset)
{
    DWORD result;
    HANDLE file_handle = (HANDLE)handle;
    result = SetFilePointer(file_handle, offset, NULL, FILE_BEGIN);
    if (result != INVALID_SET_FILE_POINTER) {
        return offset;
    }
    ASSERT(0);
    return win32_errcode_result_get(GetLastError());
}

// @brief 文件信息
uint32_t app_file_size(file_fd_t handle)
{
    LARGE_INTEGER fileSize;
    fileSize.QuadPart = 0;
    HANDLE file_handle = (HANDLE)handle;
    GetFileSizeEx(file_handle, &fileSize);
    return fileSize.QuadPart;
}

