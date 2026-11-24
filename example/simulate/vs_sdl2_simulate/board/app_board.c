// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG              "board"
#include "app_board.h"
#include "htd_device.h"
#include "htd_components.h"
#include "app_filesystem.h"

// @brief 初始化内存
static void app_heap_init(void)
{
    uint8_t *p_ptr = (uint8_t*)malloc(1024*1024*16);
    ASSERT(p_ptr);

    int32_t ret;
    htd_memheap_dev_t* p_dev = htd_memheap_ff_init(p_ptr, 1024*1024*16);
    ret = htd_memheap_sram_dev_add(p_dev);
    if (ret != 0) {
        free(p_ptr);
        ASSERT(0);
    }
}

// @brief 初始化板子
int app_board_init(void)
{
    // 内存 heap 初始化
    app_heap_init();
#ifdef HTD_CONSOLE_ENABLE
    // 控制台
    app_console_init();
    app_console_read_start();
#endif
    // 初始化设备
    htd_dev_init();
    // 初始化文件系统
    app_filesystem_init();
    // 初始化norflash
    htd_dev_norflash_init();
    // 初始化nandflash
    //htd_dev_nandflash_init();
    
    // 挂载文件系统
    app_filesystem_mount();
    // 初始化任务调度器
    htd_task_sch_init(32);
    
    
    return 0;
}

