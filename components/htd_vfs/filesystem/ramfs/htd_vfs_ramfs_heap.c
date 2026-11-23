// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include <htd_vfs_process.h>
#include <htd_vfs_file.h>
#include <htd_vfs_ramfs.h>
#include <htd_vfs_ramfs_heap.h>
#include <htd_vfs_list.h>

typedef struct {
    uintptr_t heap;
    htd_list_node_t node;
} ramfs_heap_item_t;

// 首次适应算法(First Fit)
typedef struct {
    uint32_t magic;
    uintptr_t start_addr;               // 内存起始地址
    size_t mem_size;                    // 内存大小
    size_t available_size;              // 可用大小
    size_t max_used_size;               // 最大的使用大小(极限值)
    htd_list_node_t node;           // 块链表
    uintptr_t end_item;
    htd_list_node_t* free_item;
} ramfs_heap_t;

#define RAMFS_HEAP_MAGIC                0x15a015a0
#define RAMFS_HEAP_MASK                 0xFFFFFFFE
#define RAMFS_HEAP_USED                 0x01
#define RAMFS_ITEM_IS_USED(item)        ((item)->heap & RAMFS_HEAP_USED)
#define RAMFS_ITEM_USED_SET(item)       ((item)->heap |= RAMFS_HEAP_USED)
#define RAMFS_ITEM_FREED_SET(item)      ((item)->heap &= RAMFS_HEAP_MASK)
#define RAMFS_HEAP_MINIALLOC            ALIGN_UP(32, sizeof(void*))
#define RAMFS_HEAP_ITEM_SIZE            ALIGN_UP(sizeof(ramfs_heap_item_t), sizeof(void*))


// @brief 断言item
static bool ramfs_heap_item_test(const ramfs_heap_item_t* p_item)
{
    ramfs_heap_t* p_heap = (ramfs_heap_t*)(p_item->heap & RAMFS_HEAP_MASK);
    return (p_heap->magic == RAMFS_HEAP_MAGIC);
}

// @brief 获取申请的内存大小
static size_t ramfs_heap_mem_size_get(const ramfs_heap_item_t* p_item)
{
    ramfs_heap_item_t* p_next = htd_list_entry(p_item->node.next, ramfs_heap_item_t, node);
    return (uintptr_t)p_next - (uintptr_t)p_item - RAMFS_HEAP_ITEM_SIZE;
}

// @brief 动态申请内存
// @param size 申请的内存大小
// @return 成功返回申请到的内存地址，失败返回NULL
void *ramfs_heap_malloc(void* p_mem, uint32_t size)
{
    ramfs_heap_t* p_heap = (ramfs_heap_t*)p_mem;
    ASSERT(p_heap != NULL);
    size = ALIGN_UP(size, sizeof(void*));
    if (size < RAMFS_HEAP_MINIALLOC) {
        size = RAMFS_HEAP_MINIALLOC;
    }
    if (size >= p_heap->available_size) {
        return NULL;
    }

    // 查找可用的大小
    htd_list_node_t *pos;
    size_t free_size = 0;
    ramfs_heap_item_t *p_item;
    for (pos=p_heap->free_item; pos!=&p_heap->node; pos=pos->next) {
        p_item = htd_list_entry(pos, ramfs_heap_item_t, node);
        ASSERT(ramfs_heap_item_test(p_item));
        if (!RAMFS_ITEM_IS_USED(p_item)) {
            free_size = ramfs_heap_mem_size_get(p_item);
            if (free_size >= size) {
                break;
            }
        }
    }
    if (free_size < size) {
        return NULL;
    }
    
    size_t used_size;
    if (free_size >= (size + RAMFS_HEAP_ITEM_SIZE + RAMFS_HEAP_MINIALLOC)) {
        // 超过一定大小，进行分割
        ramfs_heap_item_t *p_new_item = (ramfs_heap_item_t*)((uint8_t*)p_item + size + RAMFS_HEAP_ITEM_SIZE);
        print_debug("split: block[0x%p] next[0x%p] prev[0x%p] to new[0x%p]", p_item, p_item->node.next, p_item->node.prev, p_new_item);
        p_new_item->heap = (uintptr_t)p_heap;
        htd_list_insert_after(&p_new_item->node, &p_item->node);
        p_heap->available_size -= size + RAMFS_HEAP_ITEM_SIZE;
        used_size = p_heap->mem_size - p_heap->available_size;
        if (used_size > p_heap->max_used_size) {
            p_heap->max_used_size = used_size;
        }
        if (&p_item->node == p_heap->free_item) {
            p_heap->free_item = &p_new_item->node;
        }
    } else {
        p_heap->available_size -= free_size;
        used_size = p_heap->mem_size - p_heap->available_size;
        if (used_size > p_heap->max_used_size) {
            p_heap->max_used_size = used_size;
        }
    }

    p_item->heap = (uintptr_t)p_heap;
    RAMFS_ITEM_USED_SET(p_item);
    print_debug("alloc mem: memory[0x%p], block[0x%p], size: %d", (void*)((uint8_t*)p_item + RAMFS_HEAP_ITEM_SIZE), p_item, size);
    return (void *)((uintptr_t)p_item + RAMFS_HEAP_ITEM_SIZE);
}

// @brief 动态申请内存
// @param size 申请的内存大小
// @return 成功返回申请到的内存地址，失败返回NULL
void *ramfs_heap_realloc(void* p_mem, void *ptr, size_t newsize)
{
    if (newsize == 0) {
        ramfs_heap_free(ptr);
        return NULL;
    }
    newsize = ALIGN_UP(newsize, sizeof(void*));
    if (newsize < RAMFS_HEAP_MINIALLOC) {
        newsize = RAMFS_HEAP_MINIALLOC;
    }
    if (ptr == NULL) {
        return ramfs_heap_malloc(p_mem, newsize);
    }
    
    ramfs_heap_t *p_heap = (ramfs_heap_t*)p_mem;
    ASSERT(p_heap);
    
    ramfs_heap_item_t *p_item = (ramfs_heap_item_t*)((uint8_t*)ptr - RAMFS_HEAP_ITEM_SIZE);
    size_t oldsize = ramfs_heap_mem_size_get(p_item);

    ramfs_heap_item_t* p_new_item;
    ramfs_heap_item_t *p_next;
    if (newsize > oldsize) {
        p_next = htd_list_entry(p_item->node.next, ramfs_heap_item_t, node);
        ASSERT(ramfs_heap_item_test(p_next));
        if (false == RAMFS_ITEM_IS_USED(p_next)) {
            size_t nextsize = ramfs_heap_mem_size_get(p_next);
            if (nextsize + oldsize > newsize + RAMFS_HEAP_MINIALLOC) {
                // 分割
                p_heap->available_size -= (newsize - oldsize);
                size_t used_size = p_heap->mem_size - p_heap->available_size;
                if (used_size > p_heap->max_used_size) {
                    p_heap->max_used_size = used_size;
                }
                print_debug("remove block: block[0x%p]", p_next);
                if (p_heap->free_item == &p_next->node) {
                    p_heap->free_item = NULL;
                }
                htd_list_del(&p_next->node);
                
                // 将剩余部分切割成新的节点
                p_next = (ramfs_heap_item_t*)((char*)ptr + newsize);
                p_next->heap = (uintptr_t)p_heap;
                RAMFS_ITEM_FREED_SET(p_next);
                htd_list_insert_after(&p_next->node, &p_item->node);
                if (p_heap->free_item == NULL) {
                    p_heap->free_item = &p_next->node;
                }
                return ptr;
            }
        }
        
        // 重新申请内存
        void *p_new_ptr = (void *)ramfs_heap_malloc(p_mem, newsize);
        if (p_new_ptr != NULL) {
            memcpy(p_new_ptr, ptr, oldsize);
            ramfs_heap_free(ptr);
        }
        return p_new_ptr;
    }
    
    // 变小，但不够分割
    if (newsize + RAMFS_HEAP_ITEM_SIZE + RAMFS_HEAP_MINIALLOC >= oldsize) {
        return ptr;
    }
    
    // 变小，需要做分割
    p_new_item = (ramfs_heap_item_t*)(((uint8_t *)p_item) + newsize + RAMFS_HEAP_ITEM_SIZE);
    print_debug("split: block[0x%p] nextm[0x%p] prevm[0x%p] to new[0x%p]", p_item, p_item->node.next, p_item->node.prev, p_new_item);
    p_new_item->heap = (uintptr_t)p_heap;
    RAMFS_ITEM_FREED_SET(p_new_item);
    htd_list_insert_after(&p_new_item->node, &p_item->node);
    if (p_heap->free_item > &p_new_item->node) {
        p_heap->free_item = &p_new_item->node;
    }
    
    // 右边是否需要合并
    p_next = htd_list_entry(p_new_item->node.next, ramfs_heap_item_t, node);
    if (p_next > p_new_item) {
        ASSERT(ramfs_heap_item_test(p_next));
        if (false == RAMFS_ITEM_IS_USED(p_next)) {
            // 合并右边
            p_heap->available_size -= ramfs_heap_mem_size_get(p_next);
            print_debug("merge: right node 0x%p", p_next);
            htd_list_del(&p_next->node);
        }
    }
    p_heap->available_size += ramfs_heap_mem_size_get(p_new_item);
    return ptr;
}

// @brief 动态释放内存
// @param ptr 释放的内存地址
void ramfs_heap_free(void *ptr)
{
    if (ptr == NULL) {
        return ;
    }

    // 检查状态
    ramfs_heap_item_t *p_item = (ramfs_heap_item_t*)((uint8_t*)ptr - RAMFS_HEAP_ITEM_SIZE);
    print_debug("free memory: memory[0x%p], block[0x%p]", ptr, p_item);
    ASSERT(ramfs_heap_item_test(p_item));
    if (!RAMFS_ITEM_IS_USED(p_item)) {
        ASSERT(0);
    }

    // 释放
    ramfs_heap_t *p_heap = (ramfs_heap_t*)(p_item->heap & RAMFS_HEAP_MASK);
    ASSERT(p_heap && p_heap->magic == RAMFS_HEAP_MAGIC);
    RAMFS_ITEM_FREED_SET(p_item);
    p_heap->available_size += ramfs_heap_mem_size_get(p_item); // 重置有效大小
    if (p_heap->free_item > &p_item->node) {
        p_heap->free_item = &p_item->node;
    }
    
    // 判断左边是否需要将空闲节点合并
    ramfs_heap_item_t *p_next = htd_list_entry(p_item->node.next, ramfs_heap_item_t, node);
    ramfs_heap_item_t *p_prev = htd_list_entry(p_item->node.prev, ramfs_heap_item_t, node);
    if ((uintptr_t)p_prev >= p_heap->start_addr) {
        ASSERT(ramfs_heap_item_test(p_prev));
        if (false == RAMFS_ITEM_IS_USED(p_prev)) {
            print_debug("merge: left node 0x%p", p_prev);
            p_heap->available_size += RAMFS_HEAP_ITEM_SIZE;
            htd_list_del(&p_item->node);
        }
    }
    
    // 判断右边是否需要将空闲节点合并
    if ((uintptr_t)p_next < p_heap->end_item) {
        ASSERT(ramfs_heap_item_test(p_next));
        if (false == RAMFS_ITEM_IS_USED(p_next)) {
            print_debug("merge: right node 0x%p", p_next);
            p_heap->available_size += RAMFS_HEAP_ITEM_SIZE;
            htd_list_del(&p_next->node);
        }
    }
}

// @brief 判断 p_ptr 指针是否在内存池内
uint32_t ramfs_heap_ptr_is_in_range(void* p_mem, uintptr_t p_ptr)
{
    ramfs_heap_t *p_heap = (ramfs_heap_t*)p_mem;
    uintptr_t p_end = p_heap->start_addr + p_heap->mem_size;
    if (p_ptr >= p_heap->start_addr && p_ptr < p_end) {
        return 1;
    }
    return 0;
}

// @brief 获取内存大小
uint32_t ramfs_heap_size_get(void* p_mem)
{
    ramfs_heap_t* p_heap = (ramfs_heap_t*)p_mem;
    ASSERT(p_heap);
    return p_heap->mem_size;
}

// @brief 获取有效的大小
uint32_t ramfs_heap_available_size_get(void* p_mem)
{
    ramfs_heap_t* p_heap = (ramfs_heap_t*)p_mem;
    ASSERT(p_heap);
    return p_heap->available_size;
}

// @brief 初始化动态内存
void* ramfs_heap_init(void* p_start_addr, size_t size)
{
    ramfs_heap_t *p_heap = (ramfs_heap_t*)ALIGN_UP((uintptr_t)p_start_addr, sizeof(void*));
    size -= (size_t)((uint8_t*)p_heap - (uint8_t*)p_start_addr);
    size = ALIGN_DOWN(size, sizeof(void*));
    size -= ALIGN_UP(sizeof(ramfs_heap_t), sizeof(void*));
    if (size < 2*RAMFS_HEAP_ITEM_SIZE) {
        return NULL;
    }
    print_debug("memory heap: start addr 0x%08x, size %d", p_start_addr, size);
    
    memset(p_heap, 0, sizeof(ramfs_heap_t));
    p_heap->magic = RAMFS_HEAP_MAGIC;
    p_heap->start_addr = (uintptr_t)p_heap;
    p_heap->start_addr += ALIGN_UP(sizeof(ramfs_heap_t), sizeof(void*));
    p_heap->mem_size = size;
    p_heap->available_size = p_heap->mem_size - (2*RAMFS_HEAP_ITEM_SIZE);
    p_heap->max_used_size = p_heap->mem_size - p_heap->available_size;
    htd_list_node_init(&p_heap->node);
    
    // 创建 block 首节点
    ramfs_heap_item_t *p_item = (ramfs_heap_item_t*)p_heap->start_addr;
    p_item->heap = (uintptr_t)p_heap;
    RAMFS_ITEM_FREED_SET(p_item);
    htd_list_insert_after(&p_item->node, &p_heap->node);
    p_heap->free_item = &p_item->node;
    
    // 最后一个节点
    p_item = (ramfs_heap_item_t*)((uint8_t*)p_item + p_heap->available_size + RAMFS_HEAP_ITEM_SIZE);
    memset(p_item, 0, sizeof(ramfs_heap_item_t));
    p_item->heap = (uintptr_t)p_heap;
    RAMFS_ITEM_USED_SET(p_item);
    htd_list_insert_before(&p_item->node, &p_heap->node);
    print_debug("range: 0x%08x->0x%p", p_heap->start_addr, p_item);

    p_heap->end_item = (uintptr_t)p_item;
    return p_heap;
}


