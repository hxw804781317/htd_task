// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "dev"
#include "htd_device.h"


//////////////////////////////////////////////////////////////////////
HTD_SECTION_DEF(HTD_DEV_SECTION_NAME, htd_dev_t);


// @brief 查找对应的设备
htd_dev_t* htd_dev_find(const char* p_name, uint32_t identify_number)
{
    ASSERT(p_name);
    uint32_t name_len = htd_strlen(p_name) + 1;

    const uint32_t* start = (uint32_t*)HTD_SECTION_START_ADDR(HTD_DEV_SECTION_NAME);
    const uint32_t* end = (uint32_t*)HTD_SECTION_END_ADDR(HTD_DEV_SECTION_NAME);
    htd_dev_t* p_dev;
    const uint32_t* p_item = htd_section_first_item_get(start, end);
    while (p_item) {
        p_dev = (htd_dev_t*)p_item;
        if (p_dev->magic == identify_number) {
            if (htd_memcmp(p_name, p_dev->p_name, name_len) == 0) {
                return p_dev;
            }
        }
        p_item = htd_section_next_item_get(p_item, end, sizeof(htd_dev_t));
    }
    return NULL;
}

// @brief 展示所有设备
void htd_dev_list(void)
{
    const uint32_t* start = (uint32_t*)HTD_SECTION_START_ADDR(HTD_DEV_SECTION_NAME);
    const uint32_t* end = (uint32_t*)HTD_SECTION_END_ADDR(HTD_DEV_SECTION_NAME);

    const htd_dev_t* p_dev;
    const uint32_t* p_item = htd_section_first_item_get(start, end);
    print_warning("device: ");
    while (p_item) {
        p_dev = (htd_dev_t*)p_item;
        if ((p_dev->magic & HTD_DEV_MAGIC_MASK) == HTD_DEV_MAGIC_MASK) {
            print_warning("\t%s", p_dev->p_name);
        }
        p_item = htd_section_next_item_get(p_item, end, sizeof(htd_dev_t));
    }
}





//////////////////////////////////////////////////////////////////////
HTD_SECTION_DEF(DEVICE_INIT_SECTION_NAME, dev_init_t);

// @brief 初始化
int32_t htd_dev_init(void)
{
    const uint32_t* start = (uint32_t*)HTD_SECTION_START_ADDR(DEVICE_INIT_SECTION_NAME);
    const uint32_t* end = (uint32_t*)HTD_SECTION_END_ADDR(DEVICE_INIT_SECTION_NAME);
    dev_init_t* p_init;
    const uint32_t* p_item = htd_section_first_item_get(start, end);
    while (p_item) {
        p_init = (dev_init_t*)p_item;
        if (p_init->magic == DEV_INIT_MAGIC) {
            print_debug("dev init. %s", p_init->name);
            p_init->init_func();
        }
        p_item = htd_section_next_item_get(p_item, end, sizeof(dev_init_t));
    }
    return 0;
}




//////////////////////////////////////////////////////////////////////
HTD_SECTION_DEF(HTD_EVT_BROADCAST_SECTION, htd_evt_broadcast_t);

// @brief 事件广播
void htd_dev_evt_broadcast(htd_dev_evt_t evt, void* p_param, uint32_t param_len)
{
    const uint32_t *start = (const uint32_t*)HTD_SECTION_START_ADDR(HTD_EVT_BROADCAST_SECTION);
    const uint32_t *end = (const uint32_t*)HTD_SECTION_END_ADDR(HTD_EVT_BROADCAST_SECTION);
    htd_evt_broadcast_t *handle;
    const uint32_t* p_item = htd_section_first_item_get(start, end);
    while (p_item) {
        handle = (htd_evt_broadcast_t*)p_item;
        if (handle->magic == HTD_EVT_MAGIC_VAL && handle->evt == evt) {
            handle->func(p_param, param_len);
        }
        p_item = htd_section_next_item_get(p_item, end, sizeof(dev_init_t));
    }
}

