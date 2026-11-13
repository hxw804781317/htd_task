// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "memheap.sram"
#include "htd_task.h"
#include "htd_memheap_sram_dev.h"

static htd_memheap_dev_t* m_memheap_dev_table[HTD_MEMHEAP_SRAM_DEV_COUNT+1];
static htd_sem_lock_t* m_memheap_ram_lock = NULL;
static uint8_t m_memheap_ram_is_init = 0;


// @brief 添加内存设备
int32_t htd_memheap_sram_dev_add(htd_memheap_dev_t* p_dev)
{
    uint32_t i;
    if (m_memheap_ram_is_init != 0xA5) {
        for (i=0; i<ARRAY_COUNT(m_memheap_dev_table); ++i) {
            m_memheap_dev_table[i] = NULL;
        }
        m_memheap_ram_lock = htd_sem_lock_create("htd_heap_ram");
        ASSERT(m_memheap_ram_lock);
        m_memheap_ram_is_init = 0xA5;
    }
    
    for (i=0; i<ARRAY_COUNT(m_memheap_dev_table); ++i) {
        if (m_memheap_dev_table[i] == p_dev) {
            return 0;
        }
    }
    for (i=0; i<ARRAY_COUNT(m_memheap_dev_table); ++i) {
        if (m_memheap_dev_table[i] == NULL) {
            m_memheap_dev_table[i] = p_dev;
            return 0;
        }
    }
    return -1;
}

// @brief 删除内存设备
int32_t htd_memheap_sram_dev_del(htd_memheap_dev_t* p_dev)
{
    if (m_memheap_ram_is_init == false) {
        return 0;
    }

    htd_sem_lock(m_memheap_ram_lock);
    for (uint32_t i=0; i<ARRAY_COUNT(m_memheap_dev_table); ++i) {
        if (m_memheap_dev_table[i] == p_dev) {
            m_memheap_dev_table[i] = NULL;
            break;
        }
    }
    htd_sem_unlock(m_memheap_ram_lock);
    return 0;
}

// @brief 申请内存
void* htd_memheap_sram_malloc(size_t size, void* fun)
{
    void* p_ptr = NULL;
    htd_sem_lock(m_memheap_ram_lock);
    for (uint32_t i=0; i<ARRAY_COUNT(m_memheap_dev_table); ++i) {
        if (m_memheap_dev_table[i]) {
            p_ptr = m_memheap_dev_table[i]->alloc_handle(m_memheap_dev_table[i], size, fun);
            if (p_ptr) {
                break;
            }
        }
    }
    htd_sem_unlock(m_memheap_ram_lock);
    return p_ptr;
}

// @brief 申请内存
void* htd_memheap_sram_calloc(uint32_t count, size_t size, void* fun)
{
    size *= count;
    void* p_ptr = htd_memheap_sram_malloc(size, fun);
    if (p_ptr) {
        htd_memset(p_ptr, 0, size);
    }
    return p_ptr;
}

// @brief 重新申请内存; 只能在同一块内存中realloc
void* htd_memheap_sram_realloc(void *p_ptr, size_t newsize, void* fun)
{
    void* p_new_ptr = NULL;
    htd_sem_lock(m_memheap_ram_lock);
    for (uint32_t i=0; i<ARRAY_COUNT(m_memheap_dev_table); ++i) {
        if (m_memheap_dev_table[i]) {
            if (m_memheap_dev_table[i]->ctrl_handle(m_memheap_dev_table[i], HTD_MEMHEAP_CMD_PTR_TEST, p_ptr, sizeof(void*)) == 1) {
                p_new_ptr = m_memheap_dev_table[i]->realloc_handle(m_memheap_dev_table[i], p_ptr, newsize, fun);
                break;
            }
        }
    }
    htd_sem_unlock(m_memheap_ram_lock);
    return p_new_ptr;
}

// @brief 释放内存
void htd_memheap_sram_free(const void *p_ptr)
{
    htd_sem_lock(m_memheap_ram_lock);
    for (uint32_t i=0; i<ARRAY_COUNT(m_memheap_dev_table); ++i) {
        if (m_memheap_dev_table[i]) {
            if (m_memheap_dev_table[i]->ctrl_handle(m_memheap_dev_table[i], HTD_MEMHEAP_CMD_PTR_TEST, (void*)p_ptr, sizeof(void*)) == 1) {
                m_memheap_dev_table[i]->free_handle((void*)p_ptr);
                break;
            }
        }
    }
    htd_sem_unlock(m_memheap_ram_lock);
}

// @brief 显示所有内存信息
void htd_memheap_sram_dump(void)
{
    htd_sem_lock(m_memheap_ram_lock);
    for (uint32_t i=0; i<ARRAY_COUNT(m_memheap_dev_table); ++i) {
        if (m_memheap_dev_table[i]) {
            m_memheap_dev_table[i]->ctrl_handle(m_memheap_dev_table[i], HTD_MEMHEAP_CMD_DUMP, NULL, 0);
        }
    }
    htd_sem_unlock(m_memheap_ram_lock);
}




//////////////////////////////////////////////////////////////////////
#if defined(_WIN32) && defined(HTD_CONSOLE_ENABLE) && DEBUG_LVL > 0 
#include "htd_console_process.h"

static void htd_heap_test(void)
{
    int32_t ret, i;
    size_t heap_size = 1024*1024;
    void* p_start_addr = malloc(heap_size);
    ASSERT(p_start_addr);

    srand(htd_time_ms_get());
    htd_memheap_dev_t* p_dev = htd_memheap_ff_init(p_start_addr, heap_size);
    ASSERT(p_dev);
    
    ret = htd_memheap_sram_dev_add(p_dev);
    ASSERT(ret == 0);

    uint32_t size;
    void* p_table[8];
    for (i=0; i<ARRAY_COUNT(p_table); ++i) {
        size = 1 + rand() % (2*1024);
        print_warning("malloc. idx=%d. size=%d", i, size);
        p_table[i] = htd_memheap_sram_malloc(size, __FUNCTION__);
        if (p_table[i]) {
            htd_memset(p_table[i], 0xff, size);
        }
    }

    // 随机释放一个，并重新申请
    uint32_t idx;
    for (i=0; i<1000000; ++i) {
        idx = rand() % ARRAY_COUNT(p_table);
        if (p_table[idx]) {
            if (idx & 0x01) {
                print_warning("free. idx=%d. 0x%p", idx, p_table[idx]);
                htd_memheap_sram_free(p_table[idx]);
                size = 1 + rand() % (2*1024);
                print_warning("malloc. idx=%d. size=%d", idx, size);
                p_table[idx] = htd_memheap_sram_malloc(size, __FUNCTION__);
                if (p_table[idx] == NULL) {
                    htd_memheap_sram_dump();
                    ASSERT(0);
                    continue;
                }
            } else {
                size = 1 + rand() % (2*1024);
                print_warning("realloc. idx=%d, size=%d", idx, size);
                p_table[idx] = htd_memheap_sram_realloc(p_table[idx], size, __FUNCTION__);
                if (p_table[idx] == NULL) {
                    htd_memheap_sram_dump();
                    ASSERT(0);
                    continue;
                }
            }
            htd_memset(p_table[idx], 0xff, size);
        }
    }
    
    htd_memheap_sram_dump();
    htd_memheap_sram_dev_del(p_dev);
    free(p_start_addr);
}


// @brief 调试命令
static int cmd_heap(int argc, char **argv)
{
    if (argc != 2) {
        print_error("Usage: %s test", argv[0]);
        return 0;
    }
    if (memcmp(argv[1], "test", 5) == 0) {
        htd_heap_test();
    } else {
        print_error("Usage: %s test", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(cmd_heap, heap);
#endif /**/


