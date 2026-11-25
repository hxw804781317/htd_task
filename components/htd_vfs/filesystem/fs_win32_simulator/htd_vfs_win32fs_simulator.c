// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL   4
#define DEBUG_TAG              "vfs.simu"
#include <htd_vfs_port.h>

#if defined(HTD_VFS_ENABLE) && defined(_WIN32)
//////////////////////////////////////////////////////////////////////
#include <io.h>
#include <WinError.h>
#include <windows.h>

#ifndef WIN32_DIRDISK_ROOT
    #define WIN32_DIRDISK_ROOT  "./disk"
#endif

typedef struct {
    HANDLE handle;
    struct _finddata_t file_info;
    uint32_t pos;
} win32_dir_t;

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
    print_error("error. res=%d", res);
    ASSERT(0);
    for (int i=0; i<ARRAY_COUNT(errcode_table); i++) {
        if (errcode_table[i].win32err == (res & 0xFFFF)) {
            return (0 - (int)errcode_table[i].dfserr);
        }
    }
    print_error("win32 error not supported: res=%d", res);
    return -1;
}

// @brief 判断文件夹是否存在
static bool win32_dir_is_exist(const char* p_path)
{
    WIN32_FIND_DATA file_info;
    HANDLE file_handle = FindFirstFile(p_path, &file_info);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    FindClose(file_handle);
    if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return true;
    }
    return false;
}

// @brief 挂载
int vfs_win32_mount(htd_vfs_t *p_vfs, const void *data)
{
    // 文件夹不存在，创建文件夹
    if (!win32_dir_is_exist(WIN32_DIRDISK_ROOT)) {
        CreateDirectory(WIN32_DIRDISK_ROOT, NULL);
    }
    return 0;
}

// @brief 卸载
int vfs_win32_unmount(htd_vfs_t *p_vfs)
{
    return 0;
}

// @brief 获取文件系统信息
int vfs_win32_statfs(htd_vfs_t *p_vfs, htd_vfs_statfs_t *buf)
{
    return -ERROR_CODE_INVALID_STATE;
}

// @brief 拷贝目录路径
static char *win32_dirpath_dup(const char *p_src)
{
    uint32_t len = sizeof(WIN32_DIRDISK_ROOT) + 16;
    if (p_src) {
        len += htd_strlen(p_src) + 1;
    }

    char* p_path = htd_malloc(len);
    if (p_path == NULL) {
        return NULL;
    }
    len = htd_snprintf(p_path, len, "%s%s", WIN32_DIRDISK_ROOT, p_src);
    
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
int vfs_win32_open(htd_vfs_fd_t *p_fd)
{
    uint32_t oflag;
    char *p_filepath;
    int res;

    oflag = p_fd->flags;
    if (oflag & O_DIRECTORY) {
        win32_dir_t *p_wdir;
        HANDLE handle;
        int len;
        
        p_filepath = win32_dirpath_dup(p_fd->path);
        if (oflag & O_CREAT)  {
            if (! win32_dir_is_exist(p_filepath)) {
                res = CreateDirectory(p_filepath, NULL);
                if (res == 0) {
                    print_error("create dir failed. p_filepath=%s", p_filepath);
                    htd_free(p_filepath);
                    ASSERT(0);
                    return win32_errcode_result_get(GetLastError());
                }
            }
        }

        // 拼接搜索路径
        len = htd_strlen(p_filepath);
        if (p_filepath[len - 1] != '\\') {
            p_filepath[len] = '\\';
            p_filepath[len + 1] = 0;
        }
        strcat(p_filepath, "*.*");

        // 搜索第一个符合条件的文件
        p_wdir = htd_malloc(sizeof(win32_dir_t));
        ASSERT(p_wdir != NULL);
        htd_memset(p_wdir, 0, sizeof(win32_dir_t));
        if ((handle = (HANDLE)_findfirst(p_filepath, &p_wdir->file_info)) == (HANDLE)-1) {
            htd_free(p_wdir);
            htd_free(p_filepath);
            return 0;
        }
        p_wdir->pos = 1;
        p_wdir->handle = handle;
        p_fd->data = (void *)p_wdir;
        htd_free(p_filepath);
        return 0;
    }

    long create_type = OPEN_EXISTING;   // 文件必须已经存在
    if (oflag & O_CREAT) {              // 如文件不存在则创建它
        create_type = OPEN_ALWAYS;
    } 
    if (oflag & O_TRUNC) {              // 现有文件缩短为零长度
        create_type = CREATE_ALWAYS;
    }

    p_filepath = win32_dirpath_dup(p_fd->path);
    HANDLE file_handle = CreateFile(p_filepath, GENERIC_READ|GENERIC_WRITE, 0, NULL, create_type, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        print_error("create file failed. filepath=%s", p_filepath);
        htd_free(p_filepath);
        ASSERT(0);
        return win32_errcode_result_get(GetLastError());
    }
    htd_free(p_filepath);
    
    p_fd->data = (void *)file_handle;
    p_fd->pos = 0;
    LARGE_INTEGER fileSize;
    fileSize.QuadPart = 0;
    GetFileSizeEx(file_handle, &fileSize);
    p_fd->size = (uint32_t)fileSize.QuadPart;
    if (oflag & O_APPEND){
        p_fd->pos = p_fd->size;
    } 
    SetFilePointer(file_handle, p_fd->pos, NULL, FILE_BEGIN);
    return 0;
}

// @brief 关闭文件
int vfs_win32_close(htd_vfs_fd_t *p_fd)
{
    if (p_fd->flags & O_DIRECTORY) {
        win32_dir_t *p_wdir = (win32_dir_t*)(p_fd->data);
        if (p_wdir) {
            _findclose((intptr_t)p_wdir->handle);
            htd_free(p_wdir);
        }
    } else {
        HANDLE file_handle = (HANDLE)(p_fd->data);
        if (file_handle) {
            CloseHandle(file_handle);
            p_fd->data = NULL;
        }
    }
    return 0;
}

// @brief IO 控制
int vfs_win32_ioctl(htd_vfs_fd_t *p_fd, int cmd, void *args)
{
    return -ERROR_CODE_INVALID_STATE;
}

// @brief 读文件数据
ssize_t vfs_win32_read(htd_vfs_fd_t *p_fd, void *p_buff, size_t buff_size)
{
    DWORD read_len = 0;
    HANDLE file_handle = (HANDLE)(p_fd->data);
    SetFilePointer(file_handle, p_fd->pos, NULL, FILE_BEGIN);
    if (!ReadFile(file_handle, p_buff, buff_size, &read_len, NULL)) {
        print_error("buff_size=%d, pos=%u, size=%u, read_len=%u", buff_size, p_fd->pos, p_fd->size, read_len);
        ASSERT(0);
        return win32_errcode_result_get(GetLastError());
    }
    return read_len;
}

// @brief 写文件数据
ssize_t vfs_win32_write(htd_vfs_fd_t *p_fd, const void *p_buff, size_t buff_len)
{
    DWORD write_len = 0;
    HANDLE file_handle = (HANDLE)(p_fd->data);
    SetFilePointer(file_handle, p_fd->pos, NULL, FILE_BEGIN);
    if (!WriteFile(file_handle, p_buff, buff_len, &write_len, NULL)) {
        ASSERT(0);
        return win32_errcode_result_get(GetLastError());
    }
    return write_len;
}

// @brief 刷新文件
int vfs_win32_flush(htd_vfs_fd_t *p_fd)
{
    return 0;
}

// @brief 偏移文件位置
htd_vfs_off_t vfs_win32_lseek(htd_vfs_fd_t *p_fd, htd_vfs_off_t offset)
{
    if (p_fd->type == FT_DIRECTORY) {
        return 0;
    }
    
    DWORD result;
    HANDLE file_handle = (HANDLE)(p_fd->data);
    result = SetFilePointer(file_handle, offset, NULL, FILE_BEGIN);
    if (result != INVALID_SET_FILE_POINTER) {
        return offset;
    }
    ASSERT(0);
    return win32_errcode_result_get(GetLastError());
}

// @brief 迭代目录信息
int vfs_win32_getdents(htd_vfs_fd_t *p_fd, htd_vfs_dirent_t *p_entry)
{
    win32_dir_t *p_wdir = (win32_dir_t*)(p_fd->data);
    if (p_wdir == NULL) {
        return 0;
    }

    if (p_wdir->pos == 0) {
        int32_t ret = _findnext((intptr_t)p_wdir->handle, &p_wdir->file_info);
        if (ret != 0) {
            int result = GetLastError();
            if (result == ERROR_NO_MORE_FILES) {
                return 0;
            } 
            print_error("findnext failed. result=%d, ret=%d", result, ret);
            return win32_errcode_result_get(result);
        }
    } else {
        p_wdir->pos--;
    }
    
    if (p_wdir->file_info.attrib & _A_SUBDIR) {
        p_entry->d_type = DT_DIR;
    } else {
        p_entry->d_type = DT_REG;
    }
    p_entry->d_namlen = htd_strlen(p_wdir->file_info.name);
    htd_strncpy(p_entry->d_name, p_wdir->file_info.name, sizeof(p_entry->d_name));
    p_entry->d_reclen = (uint16_t)sizeof(htd_vfs_dirent_t);
    return sizeof(htd_vfs_dirent_t);
}

// @brief 删除文件/文件夹
int vfs_win32_unlink(htd_vfs_t *p_vfs, const char *p_path)
{
    char *p_filepath = win32_dirpath_dup(p_path);
    ASSERT(p_filepath);

    do {
        int32_t result = GetFileAttributes(p_filepath);
        if (result == INVALID_FILE_ATTRIBUTES) {
            break;
        }

        if (result & FILE_ATTRIBUTE_DIRECTORY){
            if (RemoveDirectory(p_filepath) == false) {
                break;
            }
        } else  {
            if (_unlink(p_filepath) < 0) {
                break;
            }
        }
        htd_free(p_filepath);
        return 0;
    } while (0);

    htd_free(p_filepath);
    ASSERT(0);
    return win32_errcode_result_get(GetLastError());
}

// @brief 重命名文件/文件夹
int vfs_win32_rename(htd_vfs_t *p_vfs, const char *p_old_path, const char *p_new_path)
{
    char* p_ofp = win32_dirpath_dup(p_old_path);
    ASSERT(p_ofp);
    
    char* p_nfp = win32_dirpath_dup(p_new_path);
    ASSERT(p_nfp);

    int result = MoveFile(p_ofp, p_nfp);        // MoveFile 失败返回0
    htd_print("move file failed. result=%d, %s->%s\r\n", result, p_ofp, p_nfp);
    htd_free(p_ofp);
    htd_free(p_nfp);
    
    if (result == 0) {
        ASSERT(0);
        return win32_errcode_result_get(GetLastError());
    }
    return 0;
}

// @brief 文件信息
int vfs_win32_stat(htd_vfs_t *p_vfs, const char *p_path, htd_vfs_stat_t *p_info)
{
    char *p_filepath = win32_dirpath_dup(p_path);
    ASSERT(p_filepath);
    
    WIN32_FIND_DATA file_info;
    HANDLE file_handle = FindFirstFile(p_filepath, &file_info);
    if (file_handle == INVALID_HANDLE_VALUE) {
        htd_free(p_filepath);
        DWORD dwError = GetLastError();
        if (dwError == ERROR_FILE_NOT_FOUND || dwError == ERROR_PATH_NOT_FOUND) {
            // 文件不存在或者路径不存在
            return -ERROR_CODE_NOT_FOUND;
        }
        ASSERT(0);
        return win32_errcode_result_get(GetLastError());
    }
    htd_free(p_filepath);
    FindClose(file_handle);
    
    p_info->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
    if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        p_info->st_mode &= ~S_IFREG;
        p_info->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
    }
    p_info->st_size = file_info.nFileSizeLow;
    
    /* get st_mtime. */
    LARGE_INTEGER time_tmp;
    time_tmp.LowPart = file_info.ftLastWriteTime.dwLowDateTime;
    time_tmp.HighPart = file_info.ftLastWriteTime.dwHighDateTime;
    /* removes the diff between 1970 and 1601. */
    time_tmp.QuadPart -= 11644473600000 * 10000;
    /* converts back from 100-nanoseconds to seconds. */
    time_tmp.QuadPart /= 10UL * 1000 * 1000;
    p_info->st_mtime = time_tmp.QuadPart;
    return 0;
}
HTD_VFS_API_REGISTER(HTD_VFS_TYPE_WIN32, \
                       vfs_win32_open, vfs_win32_close, vfs_win32_ioctl, \
                       vfs_win32_read, vfs_win32_write, vfs_win32_lseek, vfs_win32_flush, \
                       vfs_win32_getdents, vfs_win32_unlink, vfs_win32_stat, \
                       vfs_win32_rename, vfs_win32_mount, vfs_win32_unmount, NULL, vfs_win32_statfs);
#endif /*defined(HTD_VFS_ENABLE) && defined(_WIN32)*/


