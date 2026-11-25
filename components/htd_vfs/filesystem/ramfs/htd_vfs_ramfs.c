// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL   4
#define DEBUG_TAG              "vfs.ramfs"
#include <htd_vfs_process.h>
#include <htd_vfs_file.h>
#include <htd_vfs_ramfs.h>
#include <htd_vfs_ramfs_heap.h>


#if defined(HTD_VFS_ENABLE) && defined(HTD_VFS_USING_RAMFS)
//////////////////////////////////////////////////////////////////////
#ifndef _WIN32
    #pragma anon_unions
#endif
typedef struct vfs_ramfs_s vfs_ramfs_t;
typedef struct ramfs_dirent_s ramfs_dirent_t;
struct ramfs_dirent_s {
    vfs_ramfs_t *p_fsram;
    ramfs_dirent_t* parent;
    ramfs_dirent_t* brother;
    char* p_name;
    uint16_t name_len;
    uint8_t type;               // DT_REG
    union {
        ramfs_dirent_t* child;
        uint8_t *data;
    };
    size_t data_size; 
};

typedef struct {
    htd_slist_node_t slist;
    void* p_heap;
    void* p_mem_start;
    void* p_mem_end;
}mem_heap_node_t;

struct vfs_ramfs_s {
    uint32_t magic;
    ramfs_dirent_t root;
    htd_slist_node_t slist;
};
#define VFS_RAMFS_MAGIC     0x5A6A7A8A



//////////////////////////////////////////////////////////////////////
// @brief 申请内存
static void* fsram_malloc(vfs_ramfs_t* p_fsram, uint32_t malloc_size)
{
    mem_heap_node_t* p_node;
    htd_slist_node_t* p_pos;
    void* p_ptr = NULL;
    htd_slist_for_each(p_pos, &p_fsram->slist) {
        p_node = htd_slist_entry(p_pos, mem_heap_node_t, slist);
        p_ptr = ramfs_heap_malloc(p_node->p_heap, malloc_size);
        if (p_ptr) {
            break;
        }
    }
    return p_ptr;
}

// @brief 释放内存
static void fsram_free(vfs_ramfs_t* p_fsram, void* p_mem)
{
    mem_heap_node_t* p_node;
    htd_slist_node_t* p_pos;
    htd_slist_for_each(p_pos, &p_fsram->slist) {
        p_node = htd_slist_entry(p_pos, mem_heap_node_t, slist);
        if (ramfs_heap_ptr_is_in_range(p_node->p_heap, (uintptr_t)p_mem)) {
            ramfs_heap_free(p_mem);
            break;
        }
    }
}

// @brief 重新申请内存
static void* fsram_relloc(vfs_ramfs_t* p_fsram, void* p_mem, uint32_t relloc_size)
{
    mem_heap_node_t* p_node;
    htd_slist_node_t* p_pos;
    void* p_ptr = NULL;
    htd_slist_for_each(p_pos, &p_fsram->slist) {
        p_node = htd_slist_entry(p_pos, mem_heap_node_t, slist);
        if (p_node->p_mem_start <= p_mem && p_mem < p_node->p_mem_end) {
            p_ptr = ramfs_heap_realloc(p_node->p_heap, p_mem, relloc_size);
            break;
        }
    }
    if (p_ptr == NULL) {
        htd_slist_for_each(p_pos, &p_fsram->slist) {
            p_node = htd_slist_entry(p_pos, mem_heap_node_t, slist);
            if (!(p_node->p_mem_start <= p_mem && p_mem < p_node->p_mem_end)) {
                p_ptr = ramfs_heap_malloc(p_node->p_heap, relloc_size);
                if (p_ptr) {
                    htd_memcpy(p_ptr, p_mem, relloc_size);
                    fsram_free(p_fsram, p_mem);
                    break;
                }
            }
        }
    }
    return p_ptr;
}



//////////////////////////////////////////////////////////////////////
// @brief 判断路径是否已经到尾部
static inline bool path_is_end(const char end)
{
    return (end == 0 || end == '/');
}

// @brief 创建文件节点
static ramfs_dirent_t* ramfs_dirent_node_create(vfs_ramfs_t *p_fsram, const char* p_name, uint8_t type)
{
    uint32_t name_len = htd_strlen(p_name) + 1;
    ramfs_dirent_t *p_entry = (ramfs_dirent_t*)fsram_malloc(p_fsram, sizeof(ramfs_dirent_t));
    if (p_entry) {
        htd_memset(p_entry, 0, sizeof(ramfs_dirent_t));
        p_entry->p_name = fsram_malloc(p_fsram, name_len);
        if (p_entry->p_name == NULL) {
            fsram_free(p_fsram, p_entry);
            return NULL;
        }
        htd_memcpy(p_entry->p_name, p_name, name_len);
        p_entry->name_len = name_len - 1;
        p_entry->p_fsram = p_fsram;
        p_entry->type = type;
    }
    return p_entry;
}

// @brief 销毁文件节点
static void ramfs_dirent_node_destroy(ramfs_dirent_t* p_dirent)
{
    if (p_dirent) {
        if (p_dirent->p_name) {
            fsram_free(p_dirent->p_fsram, p_dirent->p_name);
        }
        if (p_dirent->type != DT_DIR && p_dirent->data) {
            fsram_free(p_dirent->p_fsram, p_dirent->data);
        }
        fsram_free(p_dirent->p_fsram, p_dirent);
    }
}

// @brief 在兄弟节点中寻找
static ramfs_dirent_t *vfs_ramfs_find_dirent_in_brother(ramfs_dirent_t* p_dirent, const char* path)
{
    p_dirent = p_dirent->brother;
    while (p_dirent) {
        if (htd_memcmp(p_dirent->p_name, path, p_dirent->name_len) == 0 && 
            path_is_end(path[p_dirent->name_len])) {
            return p_dirent;
        }
        p_dirent = p_dirent->brother;
    }
    return NULL;
}

// @brief 查找对应节点
static ramfs_dirent_t *vfs_ramfs_find_dirent(vfs_ramfs_t *p_fsram, const char* path, const char* path_end)
{
    if (path[0] != '/') {
        return NULL;
    }
    path++;

    ramfs_dirent_t* p_find = NULL;
    ramfs_dirent_t* p_dirent = p_fsram->root.child;
    //vfs_print("----dirent find---path=%s\n", path);
    while (p_dirent && path + p_dirent->name_len <= path_end) {
        if (htd_memcmp(p_dirent->p_name, path, p_dirent->name_len) || false == path_is_end(path[p_dirent->name_len])) {
            //vfs_print("name_len=%d, path=%s, name=%s\n", p_dirent->name_len, path, p_dirent->p_name);
            p_dirent = vfs_ramfs_find_dirent_in_brother(p_dirent, path);
            if (p_dirent == NULL) {
                return NULL;
            }
        }
        p_find = p_dirent;
        path += (p_dirent->name_len + 1);
        p_dirent = p_dirent->child;
        //vfs_print("ramfs find. name=%s, path=%s\n", p_find->p_name, path);
    }
    if (path < path_end) {
        return NULL;
    }
    return p_find;
}

// @brief 添加节点
static bool vfs_ramfs_dirent_insert(vfs_ramfs_t *p_fsram, const char* path, ramfs_dirent_t* p_dirent)
{
    const char* p_end = htd_vfs_file_basename_get(path);
    p_end--;
    if (p_end < path) {
        htd_print("error. p_end=%p, path=%p\n", p_end, path);
        return false;
    } 

    ramfs_dirent_t *p_parent = NULL;
    if (p_end == path) {
        p_parent = &p_fsram->root;
    } else {
        p_parent = vfs_ramfs_find_dirent(p_fsram, path, p_end);
        if (p_parent == NULL || p_parent->type != DT_DIR) {
            return false;
        }
    }
    
    p_dirent->parent = p_parent;
    if (p_parent->child == NULL) {
        p_parent->child = p_dirent;
    } else {
        ramfs_dirent_t* prev = p_parent;
        ramfs_dirent_t* next = p_parent->child;
        while (1) {
            if (p_dirent->type > next->type || htd_memcmp(p_dirent->p_name, next->p_name, next->name_len) < 0) {
                if (prev == p_parent) {
                    prev->child = p_dirent;
                } else {
                    prev->brother = p_dirent;
                }
                p_dirent->brother = next;
                break;
            }
            // 最后一个了
            if (next->brother == NULL) {
                next->brother = p_dirent;
                break;
            }
            prev = next;
            next = next->brother;
        }
    }
    p_parent->data_size++;
    return true;
}

// @brief 删除某个节点以及他的所有子节点
static void vfs_ramfs_dirent_delete(ramfs_dirent_t* p_dirent)
{
    // 将该节点从关联表中断开
    ramfs_dirent_t* p_parent = p_dirent->parent;
    ramfs_dirent_t* p_child = p_parent->child;
    ramfs_dirent_t* p_prev = p_parent;
    while (p_child) {
        if (p_child == p_dirent) {
            if (p_prev == p_parent) {
                p_parent->child = p_parent->child->brother;
            } else {
                p_prev->brother = p_child->brother;
            }
            break;
        }
        p_prev = p_child;
        p_child = p_child->brother;
    }

    // 将该节点的子节点全部删除
    if (p_dirent->type == DT_DIR) {
        p_child = p_dirent->child;
        while (p_child) {
            p_prev = p_child;
            p_child = p_child->brother;
            vfs_ramfs_dirent_delete(p_prev);
        }
    } 

    // 销毁该节点
    ramfs_dirent_node_destroy(p_dirent);
    if (p_parent->data_size) {
        p_parent->data_size--;
    }
}



//////////////////////////////////////////////////////////////////////
// @brief 挂载
static int vfs_ramfs_mount(htd_vfs_t *p_vfs, const void *data)
{
    if (data == NULL) {
        return -ERROR_CODE_FAILED;
    }
    
    vfs_ramfs_t* p_fsram = (vfs_ramfs_t*)data;
    ASSERT(p_fsram->magic == VFS_RAMFS_MAGIC);
    p_vfs->p_fs_dev = (void*)p_fsram;

    if (p_fsram->root.p_name) {
        fsram_free(p_fsram, p_fsram->root.p_name);
    }
    uint32_t path_len = htd_strlen(p_vfs->p_path) + 1;
    p_fsram->root.p_name = (char*)fsram_malloc(p_fsram, path_len);
    ASSERT(p_fsram->root.p_name);
    htd_memcpy(p_fsram->root.p_name, p_vfs->p_path, path_len);
    return ERROR_CODE_SUCCESS;
}

// @brief 卸载
static int vfs_ramfs_unmount(htd_vfs_t *p_vfs)
{
    p_vfs->p_fs_dev = NULL;
    return 0;
}

// @brief 查看文件系统信息
static int vfs_ramfs_statfs(htd_vfs_t *p_vfs, htd_vfs_statfs_t *p_buff)
{
    ASSERT(p_buff != NULL);
    p_buff->f_bsize  = 512;
    p_buff->f_blocks = 0;
    p_buff->f_bfree = 0;

    mem_heap_node_t* p_node;
    htd_slist_node_t* p_pos;
    vfs_ramfs_t *p_fsram = (vfs_ramfs_t*)p_vfs->p_fs_dev;
    ASSERT(p_fsram != NULL);
    htd_slist_for_each(p_pos, &p_fsram->slist) {
        p_node = htd_slist_entry(p_pos, mem_heap_node_t, slist);
        p_buff->f_blocks += ramfs_heap_size_get(&p_node->p_heap) / p_buff->f_bsize;
        p_buff->f_bfree += ramfs_heap_available_size_get(&p_node->p_heap) / p_buff->f_bsize;
    }
    return ERROR_CODE_SUCCESS;
}

// @brief IO 控制
static int vfs_ramfs_ioctl(htd_vfs_fd_t *p_fd, int cmd, void *args)
{
    if (cmd == VFS_F_TRUNCATE) {                    // 将文件截短大小
        return ERROR_CODE_SUCCESS;
    }

    ramfs_dirent_t *p_entry = (ramfs_dirent_t *)p_fd->data;
    if (cmd == VFS_F_RESERVE_CONT_SPACE) {          // 设置文件大小
        uint32_t size = (uint32_t)args;
        if (p_entry->data_size >= size) {
            return 0;
        }
        uint8_t *ptr = NULL;
        vfs_ramfs_t *p_fsram = p_entry->p_fsram;
        if (p_entry->data) {
            ptr = fsram_relloc(p_fsram, p_entry->data, size);
        } else {
            ptr = fsram_malloc(p_fsram, size);
        }
        if (ptr == NULL) {
            return -ERROR_CODE_NO_MEM;
        }
        p_entry->data = ptr;
        p_entry->data_size = size;
        p_fd->size = size;
        return ERROR_CODE_SUCCESS;
    } 
    
    if (cmd == VFS_F_PHY_ADDR_GET) {                // 获取文件数据地址
        if (args) {
            *(uint32_t*)args = (uint32_t)&p_entry->data[p_fd->pos];
        }
    }
    return -ERROR_CODE_NOT_SUPPORTED;
}

// @brief 在文件系统中查找对应的节点
static ramfs_dirent_t *vfs_ramfs_lookup(vfs_ramfs_t *p_fsram, const char *path)
{
    const char* p_path_end = path + htd_strlen(path);
    if (p_path_end - path == 1 && path[0] == '/') {
        return &p_fsram->root;
    }
    return vfs_ramfs_find_dirent(p_fsram, path, p_path_end);
}

// @brief 读数据
static ssize_t vfs_ramfs_read(htd_vfs_fd_t *p_fd, void *p_buff, size_t buff_len)
{
    ramfs_dirent_t *p_entry = (ramfs_dirent_t*)p_fd->data;
    ASSERT(p_entry != NULL);
    if (p_entry->type == DT_DIR) {
        return ERROR_CODE_SUCCESS;
    }
    
    htd_memcpy(p_buff, p_entry->data + p_fd->pos, buff_len);
    return buff_len;
}

// @brief 写数据
static ssize_t vfs_ramfs_write(htd_vfs_fd_t *p_fd, const void *p_buff, size_t buff_len)
{
    ramfs_dirent_t *p_entry = (ramfs_dirent_t *)p_fd->data;
    ASSERT(p_entry != NULL);
    if (p_entry->type == DT_DIR) {
        return ERROR_CODE_SUCCESS;
    }

    vfs_ramfs_t *p_fsram = p_entry->p_fsram;
    ASSERT(p_fsram != NULL);
    if (buff_len > (size_t)(p_fd->size - p_fd->pos)) {
        // FIXME: 这里太浪费效率了
        uint8_t *ptr = fsram_relloc(p_fsram, p_entry->data, p_fd->pos + buff_len);
        if (ptr == NULL) {
            return ERROR_CODE_SUCCESS;
        }
        p_entry->data = ptr;
        p_entry->data_size = p_fd->pos + buff_len;
    }
    if (buff_len > 0) {
        htd_memcpy(p_entry->data + p_fd->pos, p_buff, buff_len);
    }
    return buff_len;
}

// @brief 文件位置
static htd_vfs_off_t vfs_ramfs_lseek(htd_vfs_fd_t *p_fd, htd_vfs_off_t offset)
{
    if (offset <= p_fd->size) {
        p_fd->pos = offset;
        return p_fd->pos;
    }
    return (-ERROR_CODE_INVALID_STATE);
}

// @brief 关闭
static int vfs_ramfs_close(htd_vfs_fd_t *p_fd)
{
    p_fd->data = NULL;
    return 0;
}

// @brief 打开
static int vfs_ramfs_open(htd_vfs_fd_t *p_fd)
{
    ramfs_dirent_t *p_entry;
    htd_vfs_t *p_vfs = (htd_vfs_t*)p_fd->data;
    vfs_ramfs_t *p_fsram = (vfs_ramfs_t*)p_vfs->p_fs_dev;
    ASSERT(p_fsram != NULL);
    if (p_fd->flags & O_DIRECTORY) {
        p_entry = vfs_ramfs_lookup(p_fsram, p_fd->path);
        if (p_fd->flags & O_CREAT) {
            if (p_entry != NULL) {
                return ERROR_CODE_SUCCESS;
            }
            p_entry = ramfs_dirent_node_create(p_fsram, htd_vfs_file_basename_get(p_fd->path), DT_DIR);
            if (p_entry == NULL) {
                return -ERROR_CODE_NO_MEM; 
            }
            if (false == vfs_ramfs_dirent_insert(p_fsram, p_fd->path, p_entry)) {
                htd_print("ramfs insert dirent failed. path=%s\n", p_fd->path);
                ramfs_dirent_node_destroy(p_entry);
                return -ERROR_CODE_INVALID_DATA;
            }
        } else {
            if (p_entry == NULL) {
                return -ERROR_CODE_NULL;
            }
        }
    } else {
        p_entry = vfs_ramfs_lookup(p_fsram, p_fd->path);
        if (p_entry == &p_fsram->root)  {
            return -ERROR_CODE_NULL;
        }
        if (p_entry == NULL) {
            if (p_fd->flags & O_CREAT || p_fd->flags & O_WRONLY) {
                p_entry = ramfs_dirent_node_create(p_fsram, htd_vfs_file_basename_get(p_fd->path), DT_REG);
                if (p_entry == NULL) {
                    return -ERROR_CODE_NO_MEM;
                }
                if (false == vfs_ramfs_dirent_insert(p_fsram, p_fd->path, p_entry)) {
                    ramfs_dirent_node_destroy(p_entry);
                    return -ERROR_CODE_INVALID_DATA;
                }
            } else {
                return -ERROR_CODE_NULL;
            }
        } else {
            if (p_entry->type == DT_DIR) {
                return -ERROR_CODE_INVALID_DATA;
            }
            if (p_fd->flags & O_TRUNC) {
                p_entry->data_size = 0;
                if (p_entry->data != NULL) {
                    fsram_free(p_fsram, p_entry->data);
                    p_entry->data = NULL;
                }
            }
        }
    }
    
    p_fd->data = p_entry;
    p_fd->size = p_entry->data_size;
    if (p_fd->flags & O_APPEND) {
        p_fd->pos = p_fd->size;
    } else {
        p_fd->pos = 0;
    }
    return ERROR_CODE_SUCCESS;
}

// @brief 查看文件信息
static int vfs_ramfs_stat(htd_vfs_t *p_vfs, const char *path, htd_vfs_stat_t *p_info)
{
    vfs_ramfs_t *p_fsram = (vfs_ramfs_t *)p_vfs->p_fs_dev;
    ramfs_dirent_t *p_entry = vfs_ramfs_lookup(p_fsram, path);
    if (p_entry == NULL) {
        return -ERROR_CODE_NULL;
    }
    p_info->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
    if (p_entry->type == DT_DIR) {
        p_info->st_mode &= ~S_IFREG;
        p_info->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
    } 
    p_info->st_size = p_entry->data_size;
    p_info->st_mtime = 0;
    return ERROR_CODE_SUCCESS;
}

// @brief 获取目录信息
static int vfs_ramfs_getdents(htd_vfs_fd_t *p_fd, htd_vfs_dirent_t *p_dir)
{
    ramfs_dirent_t *p_entry = (ramfs_dirent_t*)p_fd->data;
    vfs_ramfs_t *p_fsram  = p_entry->p_fsram;
    ASSERT(p_fsram != NULL);
    
    int32_t index = 0;
    p_entry = p_entry->child;
    while (p_entry && index < p_fd->size) {
        if (index >= p_fd->pos) {
            p_dir->d_type = p_entry->type;
            htd_strncpy(p_dir->d_name, p_entry->p_name, sizeof(p_dir->d_name));
            p_dir->d_namlen = htd_strlen(p_dir->d_name);
            p_dir->d_reclen = sizeof(htd_vfs_dirent_t);
            p_fd->pos++;
            return sizeof(htd_vfs_dirent_t);
        }
        index++;
        p_entry = p_entry->brother;
    }
    return ERROR_CODE_SUCCESS;
}

// @brief 删除
static int vfs_ramfs_unlink(htd_vfs_t *p_vfs, const char *p_path)
{
    vfs_ramfs_t *p_fsram = (vfs_ramfs_t*)p_vfs->p_fs_dev;
    ASSERT(p_fsram != NULL);

    ramfs_dirent_t *p_entry = vfs_ramfs_lookup(p_fsram, p_path);
    if (p_entry == NULL) {
        return -ERROR_CODE_NULL;
    }
    
    htd_print("unlink: %s\n", p_path);
    vfs_ramfs_dirent_delete(p_entry);
    return ERROR_CODE_SUCCESS;
}

// @brief 重命名
static int vfs_ramfs_rename(htd_vfs_t *p_vfs, const char *p_oldpath, const char *p_newpath)
{
    vfs_ramfs_t *p_fsram = (vfs_ramfs_t*)p_vfs->p_fs_dev;
    ASSERT(p_fsram != NULL);
    ramfs_dirent_t *p_entry = vfs_ramfs_lookup(p_fsram, p_newpath);
    if (p_entry != NULL) {
        return (-ERROR_CODE_NOT_FOUND);
    }
    p_entry = vfs_ramfs_lookup(p_fsram, p_oldpath);
    if (p_entry == NULL) {
        return -ERROR_CODE_NULL;
    }

    uint32_t newpath_len = htd_strlen(p_newpath);
    if (htd_strlen(p_entry->p_name) >= newpath_len) {
        htd_memcpy(p_entry->p_name, p_newpath, newpath_len+1);
        return ERROR_CODE_SUCCESS;
    }

    newpath_len++;
    fsram_free(p_fsram, p_entry->p_name);
    p_entry->p_name = fsram_malloc(p_fsram, newpath_len);
    if (p_entry->p_name == NULL) {
        return -ERROR_CODE_NO_MEM;
    }
    htd_memcpy(p_entry->p_name, p_newpath, newpath_len);
    return ERROR_CODE_SUCCESS;
}

// @brief 创建一个内存文件系统(这里最好增加资源保护)
void *htd_vfs_ramfs_create(uint8_t *p_mem_block, size_t mem_block_size)
{
    if (p_mem_block == NULL || mem_block_size < 128) {
        return NULL;
    }

    mem_block_size = ALIGN_DOWN(mem_block_size, 4); 
    vfs_ramfs_t *p_fsram = (vfs_ramfs_t *)p_mem_block;
    htd_memset(p_fsram, 0, sizeof(vfs_ramfs_t));
    mem_heap_node_t* p_node = (mem_heap_node_t*)(p_fsram + 1);
    htd_memset(p_node, 0, sizeof(mem_heap_node_t));
    
    uint8_t *p_data = (uint8_t*)(p_node + 1);
    mem_block_size = mem_block_size - sizeof(vfs_ramfs_t) - sizeof(mem_heap_node_t);
    mem_block_size = ALIGN_DOWN(mem_block_size, 4);
    p_node->p_heap = ramfs_heap_init(p_data, mem_block_size);
    if (p_node->p_heap == NULL) {
        return NULL;
    }
    p_node->p_mem_start = p_data;
    p_node->p_mem_end = p_data + mem_block_size;
    
    // 初始化
    p_fsram->magic = VFS_RAMFS_MAGIC;
    htd_slist_append(&p_fsram->slist, &p_node->slist);
    p_fsram->root.p_fsram = p_fsram;
    return p_fsram;
}

// @brief 添加内存块到文件系统中
int32_t htd_vfs_ramfs_append(void* ram_fs, uint8_t* p_mem_block, size_t mem_block_size)
{
    vfs_ramfs_t *p_fsram = (vfs_ramfs_t*)ram_fs;
    if (p_fsram == NULL || p_fsram->magic != VFS_RAMFS_MAGIC) {
        return -ERROR_CODE_INVALID_PARAM;
    }

    mem_heap_node_t* p_node = (mem_heap_node_t*)p_mem_block;
    uint8_t *p_data = (uint8_t*)(p_node + 1);
    mem_block_size = mem_block_size - sizeof(mem_heap_node_t);
    mem_block_size = ALIGN_DOWN(mem_block_size, 4);

    p_node->p_heap = ramfs_heap_init(p_data, mem_block_size);
    if (p_node->p_heap == NULL) {
        return -ERROR_CODE_INVALID_DATA;
    }
    
    p_node->p_mem_start = p_data;
    p_node->p_mem_end = p_data + mem_block_size;
    htd_slist_append(&p_fsram->slist, &p_node->slist);
    return ERROR_CODE_SUCCESS;
}
HTD_VFS_API_REGISTER(HTD_VFS_TYPE_RAMFS, \
                       vfs_ramfs_open, vfs_ramfs_close, vfs_ramfs_ioctl, \
                       vfs_ramfs_read, vfs_ramfs_write, vfs_ramfs_lseek, NULL, \
                       vfs_ramfs_getdents, vfs_ramfs_unlink, vfs_ramfs_stat, \
                       vfs_ramfs_rename, vfs_ramfs_mount, vfs_ramfs_unmount, NULL, vfs_ramfs_statfs);
#endif /*defined(HTD_VFS_ENABLE) && defined(VFS_USING_RAMFS)*/

