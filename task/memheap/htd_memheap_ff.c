// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL     4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "ff_heap"
#include "htd_task.h"
#include "htd_memheap_ff.h"

#if (defined(HTD_MEMHEAP_FF_ENABLE))
typedef struct {
    uint32_t magic;
#ifdef HTD_MEMHEAP_FF_USE_TRACE
    uint32_t serials;
#endif
    uintptr_t start_addr;               // 内存起始地址
    size_t mem_size;                    // 内存大小
    size_t available_size;              // 可用大小
    size_t max_used_size;               // 最大的使用大小(极限值)
    htd_list_node_t node;             // 块链表
    uintptr_t end_item;
    htd_list_node_t* free_item;
}memheap_ff_t;

typedef struct {
    uintptr_t heap;
    htd_list_node_t node;
#ifdef HTD_MEMHEAP_FF_USE_TRACE
    uintptr_t request_fun;
    uint32_t serials;
#endif
} memheap_ff_item_t;


#define MEMHEAP_FF_MAGIC                    0x15a015a0
#define MEMHEAP_FF_MASK                     0xFFFFFFFE
#define MEMHEAP_FF_USED                     0x01
#define MEMHEAP_FF_ITEM_IS_USED(item)       ((item)->heap & MEMHEAP_FF_USED)
#define MEMHEAP_FF_ITEM_USED_SET(item)      ((item)->heap |= MEMHEAP_FF_USED)
#define MEMHEAP_FF_ITEM_FREED_SET(item)     ((item)->heap &= MEMHEAP_FF_MASK)
#define MEMHEAP_FF_MINIALLOC                ALIGN_UP(32, sizeof(void*))
#define MEMHEAP_MEMHEAP_FF_ITEM_SIZE        ALIGN_UP(sizeof(memheap_ff_item_t), sizeof(void*))

// @brief 获取序列号
#ifdef HTD_MEMHEAP_FF_USE_TRACE
static inline uint32_t memheap_ff_serials_get(memheap_ff_t* p_heap)
{
    return p_heap->serials++;
}
#endif

// @brief 断言item
static bool memheap_ff_item_test(const memheap_ff_item_t* p_item)
{
    memheap_ff_t* p_heap = (memheap_ff_t*)(p_item->heap & MEMHEAP_FF_MASK);
    return (p_heap->magic == MEMHEAP_FF_MAGIC);
}

// @brief 获取申请的内存大小
static size_t memheap_ff_mem_size_get(const memheap_ff_item_t* p_item)
{
    memheap_ff_item_t* p_next = htd_list_entry(p_item->node.next, memheap_ff_item_t, node);
    return (uintptr_t)p_next - (uintptr_t)p_item - MEMHEAP_MEMHEAP_FF_ITEM_SIZE;
}

// @brief 申请内存
static void *memheap_ff_alloc(htd_memheap_dev_t *p_dev, size_t size, void* fun)
{
    memheap_ff_t *p_heap = p_dev->p_heap;
    ASSERT(p_heap != NULL);
    size = ALIGN_UP(size, sizeof(void*));
    if (size < MEMHEAP_FF_MINIALLOC) {
        size = MEMHEAP_FF_MINIALLOC;
    }
    if (size >= p_heap->available_size) {
        return NULL;
    }

    // 查找可用的大小
    htd_list_node_t *pos;
    size_t free_size = 0;
    memheap_ff_item_t *p_item;
    for (pos=p_heap->free_item; pos!=&p_heap->node; pos=pos->next) {
        p_item = htd_list_entry(pos, memheap_ff_item_t, node);
        ASSERT(memheap_ff_item_test(p_item));
        if (!MEMHEAP_FF_ITEM_IS_USED(p_item)) {
            free_size = memheap_ff_mem_size_get(p_item);
            if (free_size >= size) {
                break;
            }
        }
    }
    if (free_size < size) {
        return NULL;
    }
    
    size_t used_size;
    if (free_size >= (size + MEMHEAP_MEMHEAP_FF_ITEM_SIZE + MEMHEAP_FF_MINIALLOC)) {
        // 超过一定大小，进行分割
        memheap_ff_item_t *p_new_item = (memheap_ff_item_t*)((uint8_t*)p_item + size + MEMHEAP_MEMHEAP_FF_ITEM_SIZE);
        print_debug("split: block[0x%p] next[0x%p] prev[0x%p] to new[0x%p]", p_item, p_item->node.next, p_item->node.prev, p_new_item);
        p_new_item->heap = (uintptr_t)p_heap;
        htd_list_insert_after(&p_new_item->node, &p_item->node);
        p_heap->available_size -= size + MEMHEAP_MEMHEAP_FF_ITEM_SIZE;
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
    MEMHEAP_FF_ITEM_USED_SET(p_item);
#ifdef HTD_MEMHEAP_FF_USE_TRACE
    p_item->serials = memheap_ff_serials_get(p_heap);
    p_item->request_fun = (uintptr_t)fun;
#endif
    print_debug("alloc mem: memory[0x%p], block[0x%p], size: %d", (void*)((uint8_t*)p_item + MEMHEAP_MEMHEAP_FF_ITEM_SIZE), p_item, size);
    return (void *)((uintptr_t)p_item + MEMHEAP_MEMHEAP_FF_ITEM_SIZE);
}

// @brief 释放内存
static void memheap_ff_free(void *p_ptr)
{
    if (p_ptr == NULL) {
        return ;
    }

    // 检查状态
    memheap_ff_item_t *p_item = (memheap_ff_item_t*)((uint8_t*)p_ptr - MEMHEAP_MEMHEAP_FF_ITEM_SIZE);
    print_debug("free memory: memory[0x%p], block[0x%p]", p_ptr, p_item);
    ASSERT(memheap_ff_item_test(p_item));
    if (!MEMHEAP_FF_ITEM_IS_USED(p_item)) {
        ASSERT(0);
    }

    // 释放
    memheap_ff_t *p_heap = (memheap_ff_t*)(p_item->heap & MEMHEAP_FF_MASK);
    ASSERT(p_heap && p_heap->magic == MEMHEAP_FF_MAGIC);
    MEMHEAP_FF_ITEM_FREED_SET(p_item);
    p_heap->available_size += memheap_ff_mem_size_get(p_item); // 重置有效大小
    if (p_heap->free_item > &p_item->node) {
        p_heap->free_item = &p_item->node;
    }
    
    // 判断左边是否需要将空闲节点合并
    memheap_ff_item_t *p_next = htd_list_entry(p_item->node.next, memheap_ff_item_t, node);
    memheap_ff_item_t *p_prev = htd_list_entry(p_item->node.prev, memheap_ff_item_t, node);
    if ((uintptr_t)p_prev >= p_heap->start_addr) {
        ASSERT(memheap_ff_item_test(p_prev));
        if (false == MEMHEAP_FF_ITEM_IS_USED(p_prev)) {
            print_debug("merge: left node 0x%p", p_prev);
            p_heap->available_size += MEMHEAP_MEMHEAP_FF_ITEM_SIZE;
            htd_list_del(&p_item->node);
        }
    }
    
    // 判断右边是否需要将空闲节点合并
    if ((uintptr_t)p_next < p_heap->end_item) {
        ASSERT(memheap_ff_item_test(p_next));
        if (false == MEMHEAP_FF_ITEM_IS_USED(p_next)) {
            print_debug("merge: right node 0x%p", p_next);
            p_heap->available_size += MEMHEAP_MEMHEAP_FF_ITEM_SIZE;
            htd_list_del(&p_next->node);
        }
    }
}

// @brief 重新申请内存
static void *memheap_ff_realloc(htd_memheap_dev_t *p_dev, void *p_ptr, size_t newsize, void* fun)
{
    if (newsize == 0) {
        memheap_ff_free(p_ptr);
        return NULL;
    }
    newsize = ALIGN_UP(newsize, sizeof(void*));
    if (newsize < MEMHEAP_FF_MINIALLOC) {
        newsize = MEMHEAP_FF_MINIALLOC;
    }
    if (p_ptr == NULL) {
        return memheap_ff_alloc(p_dev, newsize, fun);
    }
    
    memheap_ff_t *p_heap = p_dev->p_heap;
    ASSERT(p_heap);
    
    memheap_ff_item_t *p_item = (memheap_ff_item_t*)((uint8_t*)p_ptr - MEMHEAP_MEMHEAP_FF_ITEM_SIZE);
    size_t oldsize = memheap_ff_mem_size_get(p_item);

    memheap_ff_item_t* p_new_item;
    memheap_ff_item_t *p_next;
    if (newsize > oldsize) {
        p_next = htd_list_entry(p_item->node.next, memheap_ff_item_t, node);
        ASSERT(memheap_ff_item_test(p_next));
        if (false == MEMHEAP_FF_ITEM_IS_USED(p_next)) {
            size_t nextsize = memheap_ff_mem_size_get(p_next);
            if (nextsize + oldsize > newsize + MEMHEAP_FF_MINIALLOC) {
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
                p_next = (memheap_ff_item_t*)((char*)p_ptr + newsize);
                p_next->heap = (uintptr_t)p_heap;
                MEMHEAP_FF_ITEM_FREED_SET(p_next);
                htd_list_insert_after(&p_next->node, &p_item->node);
                if (p_heap->free_item == NULL) {
                    p_heap->free_item = &p_next->node;
                }
                
            #ifdef HTD_MEMHEAP_FF_USE_TRACE
                p_item->serials = memheap_ff_serials_get(p_heap);
                p_item->request_fun = (uintptr_t)fun;
            #endif
                return p_ptr;
            }
        }
        
        // 重新申请内存
        void *p_new_ptr = (void *)memheap_ff_alloc(p_dev, newsize, fun);
        if (p_new_ptr != NULL) {
            htd_memcpy(p_new_ptr, p_ptr, oldsize);
            memheap_ff_free(p_ptr);
        }
        return p_new_ptr;
    }
    
#ifdef HTD_MEMHEAP_FF_USE_TRACE
    p_item->serials = memheap_ff_serials_get(p_heap);
    p_item->request_fun = (uintptr_t)fun;
#endif
    // 变小，但不够分割
    if (newsize + MEMHEAP_MEMHEAP_FF_ITEM_SIZE + MEMHEAP_FF_MINIALLOC >= oldsize) {
        return p_ptr;
    }
    
    // 变小，需要做分割
    p_new_item = (memheap_ff_item_t*)(((uint8_t *)p_item) + newsize + MEMHEAP_MEMHEAP_FF_ITEM_SIZE);
    print_debug("split: block[0x%p] nextm[0x%p] prevm[0x%p] to new[0x%p]", p_item, p_item->node.next, p_item->node.prev, p_new_item);
    p_new_item->heap = (uintptr_t)p_heap;
    MEMHEAP_FF_ITEM_FREED_SET(p_new_item);
    htd_list_insert_after(&p_new_item->node, &p_item->node);
    if (p_heap->free_item > &p_new_item->node) {
        p_heap->free_item = &p_new_item->node;
    }
    
    // 右边是否需要合并
    p_next = htd_list_entry(p_new_item->node.next, memheap_ff_item_t, node);
    if (p_next > p_new_item) {
        ASSERT(memheap_ff_item_test(p_next));
        if (false == MEMHEAP_FF_ITEM_IS_USED(p_next)) {
            // 合并右边
            p_heap->available_size -= memheap_ff_mem_size_get(p_next);
            print_debug("merge: right node 0x%p", p_next);
            htd_list_del(&p_next->node);
        }
    }
    p_heap->available_size += memheap_ff_mem_size_get(p_new_item);
    return p_ptr;
}

// @brief 判断 p_ptr 指针是否在内存池内
static uint32_t memheap_ff_ptr_is_in_range(htd_memheap_dev_t* p_dev, void* p_ptr)
{
    memheap_ff_t *p_heap = p_dev->p_heap;
    void* p_end = (void*)(p_heap->start_addr + p_heap->mem_size);
    if ((uintptr_t)p_ptr >= p_heap->start_addr && p_ptr < p_end) {
        return 1;
    }
    return 0;
}

// @brief 内存信息
static void memheap_ff_info(htd_memheap_dev_t* p_dev, size_t *total, size_t *used, size_t *max_used)
{
    memheap_ff_t *p_heap = p_dev->p_heap;
    if (total) {
        *total = p_heap->mem_size;
    }
    if (used) {
        *used = p_heap->mem_size - p_heap->available_size;
    }
    if (max_used) {
        *max_used = p_heap->max_used_size;
    }
}

// @brief 显示内存信息
static void memheap_ff_dump(htd_memheap_dev_t *p_dev)
{
    memheap_ff_t *p_heap = p_dev->p_heap;
    htd_print("memory heap address:\n");
    htd_print("start_addr: 0x%p\n", p_heap->start_addr);
    htd_print("free    : 0x%08x\n", p_heap->available_size);
    htd_print("max_used: 0x%08x\n", p_heap->max_used_size);
    htd_print("size    : 0x%08x\n", p_heap->mem_size);
    htd_print("end_item: 0x%08x\n", p_heap->end_item);
    htd_print("free_item:0x%08x\n", htd_list_entry(p_heap->free_item, memheap_ff_item_t, node));
    
    htd_list_node_t *pos;
    long block_size;
    memheap_ff_item_t *p_item;
    htd_print("\n--memory used information --\n");
    htd_list_for_each(pos, &p_heap->node) {
        p_item = htd_list_entry(pos, memheap_ff_item_t, node);
        if (!memheap_ff_item_test(p_item)) {
            htd_print("[0x%p - incorrect magic]\n", p_item);
            break;
        }
        block_size = memheap_ff_mem_size_get(p_item);
        if (block_size < 0) {
            break;
        }
        htd_print("[0x%p(%d)-0x%08x", p_item, MEMHEAP_FF_ITEM_IS_USED(p_item), block_size);
    #ifdef HTD_MEMHEAP_FF_USE_TRACE
        if (MEMHEAP_FF_ITEM_IS_USED(p_item)) {
            htd_print(", serials=%10u, func=%p", p_item->serials, p_item->request_fun);
        }
    #endif
        htd_print("]\n");
    }
}

// @brief 控制句柄
int32_t memheap_ff_ctrl(htd_memheap_dev_t *p_dev, htd_heap_cmd_t cmd, void* param, uint32_t param_len)
{
    if (cmd == HTD_MEMHEAP_CMD_DUMP) {
        memheap_ff_dump(p_dev);
    } else if (cmd == HTD_MEMHEAP_CMD_INFO) {
        ASSERT(param_len == sizeof(void*)*3);
        memheap_ff_info(p_dev, param, (size_t*)((uintptr_t)param+sizeof(void*)), (size_t*)((uintptr_t)param+sizeof(void*)*2));
    } else if (cmd == HTD_MEMHEAP_CMD_PTR_TEST) {
        return memheap_ff_ptr_is_in_range(p_dev, param);
    } else {
        return -1;
    }
    return 0;
}

// @brief 初始化
htd_memheap_dev_t* htd_memheap_ff_init(void *start_addr, size_t size)
{
    htd_memheap_dev_t* p_dev = (htd_memheap_dev_t*)ALIGN_UP((uintptr_t)start_addr, sizeof(void*));
    memheap_ff_t *p_heap = (memheap_ff_t*)ALIGN_UP((uintptr_t)(p_dev+1), sizeof(void*));
    size -= (size_t)((uint8_t*)p_heap - (uint8_t*)start_addr);
    size = ALIGN_DOWN(size, sizeof(void*));
    size -= ALIGN_UP(sizeof(memheap_ff_t), sizeof(void*));
    if (size < 2*MEMHEAP_MEMHEAP_FF_ITEM_SIZE) {
        return NULL;
    }
    print_debug("memory heap: start addr 0x%08x, size %d", start_addr, size);
    
    htd_memset(p_heap, 0, sizeof(memheap_ff_t));
    p_heap->magic = MEMHEAP_FF_MAGIC;
    p_heap->start_addr = (uintptr_t)p_heap;
    p_heap->start_addr += ALIGN_UP(sizeof(memheap_ff_t), sizeof(void*));
    p_heap->mem_size = size;
    p_heap->available_size = p_heap->mem_size - (2*MEMHEAP_MEMHEAP_FF_ITEM_SIZE);
    p_heap->max_used_size = p_heap->mem_size - p_heap->available_size;
    htd_list_head_init(&p_heap->node);
    
    // 创建 block 首节点
    memheap_ff_item_t *p_item = (memheap_ff_item_t*)p_heap->start_addr;
    p_item->heap = (uintptr_t)p_heap;
    MEMHEAP_FF_ITEM_FREED_SET(p_item);
#ifdef HTD_MEMHEAP_FF_USE_TRACE
    p_item->serials = memheap_ff_serials_get(p_heap);
    p_item->request_fun = 0;
#endif
    htd_list_head_init(&p_item->node);
    // 插入到最前面
    htd_list_insert_after(&p_item->node, &p_heap->node);
    p_heap->free_item = &p_item->node;
    
    // 最后一个节点
    p_item = (memheap_ff_item_t*)((uint8_t*)p_item + p_heap->available_size + MEMHEAP_MEMHEAP_FF_ITEM_SIZE);
    htd_memset(p_item, 0, sizeof(memheap_ff_item_t));
    p_item->heap = (uintptr_t)p_heap;
    MEMHEAP_FF_ITEM_USED_SET(p_item);
#ifdef HTD_MEMHEAP_FF_USE_TRACE
    p_item->serials = memheap_ff_serials_get(p_heap);
    p_item->request_fun = 0;
#endif
    htd_list_insert_before(&p_item->node, &p_heap->node);
    print_debug("range: 0x%08x->0x%p", p_heap->start_addr, p_item);

    p_heap->end_item = (uintptr_t)p_item;
    p_dev->p_heap = p_heap;
    p_dev->ctrl_handle = memheap_ff_ctrl;
    p_dev->free_handle = memheap_ff_free;
    p_dev->alloc_handle = memheap_ff_alloc;
    p_dev->realloc_handle = memheap_ff_realloc;
    return p_dev;
}
#endif /*HTD_MEMHEAP_FF_ENABLE*/

