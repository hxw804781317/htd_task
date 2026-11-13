// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.inflash"
#include "htd_device.h"
#include "htd_drv_common.h"


#define WRITE_ALIGN_MASK        (sizeof(void*) - 1)
#define WRITE_ALIGN_LEN         (WRITE_ALIGN_MASK + 1)


// @brief 获取flash信息，返回总共多少个内存堆
// @param idx 第几块flash，partition_count 获取多少块内存信息
uint32_t htd_inflash_partition_get(htd_flash_partition_t* p_info, uint32_t partition_count, uint32_t idx)
{
    uint32_t saddr = FLASH_BASE;
    htd_flash_partition_t* p_start = p_info;

    // 只有一块，每页2Kb
    if (idx == 0) {
        if (partition_count) {
            p_info->addr = saddr;
            p_info->size = LL_GetFlashSize() * 1024;
            p_info->erase_unit = FLASH_PAGE_SIZE;
            partition_count--;
            p_info++;
        } 
    }
    return p_info - p_start;
}

// @brief 判断是否为内部flash块
bool htd_inflash_bank_is(uint32_t start_addr, uint32_t end_addr)
{
    uint32_t flash_end_addr = FLASH_BASE + LL_GetFlashSize() * 1024;
    if (end_addr <= FLASH_BASE || start_addr >= flash_end_addr) {
        return false;
    }
    return true;
}

// @brief 读取片内flash数据
// @return 返回读取到的数据长度
int32_t htd_inflash_read(uint32_t addr, uint8_t *p_buff, size_t buff_len)
{
    uint32_t end_addr = FLASH_BASE + LL_GetFlashSize() * 1024;
    if ((addr + buff_len) > end_addr) {
        print_error("read outrange flash size! addr is (0x%p)", (void*)(addr + buff_len));
        return 0;
    }
    htd_memcpy(p_buff, (void*)addr, buff_len);
    return buff_len;
}

// @brief 最小编程单位4字节
// @return 成功返回写入的数据长度
static uint32_t drv_inflash_program(uint32_t addr, const uint8_t* p_data, uint32_t data_len)
{
    uint32_t remain_len;
    uint32_t pos = addr & WRITE_ALIGN_MASK;
    remain_len = WRITE_ALIGN_LEN - pos;
    if (data_len > remain_len) {
        data_len = remain_len;
    }
    
    uint8_t data[WRITE_ALIGN_LEN];
    if (data_len != WRITE_ALIGN_LEN) {
        // 拷贝本页数据
        addr = ALIGN_DOWN(addr, WRITE_ALIGN_LEN);
        htd_memcpy(data, (void*)addr, WRITE_ALIGN_LEN);
    } 
    htd_memcpy(&data[pos], p_data, data_len);
    if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, *(uint32_t *)data)) {
        ASSERT(0);
        return 0;
    }
    return data_len;
}

// @brief 写入片内flash数据
// @return 返回写入的数据长度
int32_t htd_inflash_write(uint32_t addr, const uint8_t *p_buff, size_t buff_len)
{
    uint32_t end_addr = FLASH_BASE + LL_GetFlashSize() * 1024;
    ASSERT(addr + buff_len <= end_addr);
    const uint8_t *p_start = p_buff;
    uint32_t write_len;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);
    // 写入数据
    while (buff_len) {
        print_debug("addr=0x%08x, buff_len=%d, %p", addr, buff_len, p_buff);
        write_len = drv_inflash_program(addr, p_buff, buff_len);
        p_buff += write_len;
        buff_len -= write_len;
        addr += write_len;
    }
    HAL_FLASH_Lock();
    return p_buff - p_start;
}

// @brief 擦除片内flash
int32_t htd_inflash_erase(uint32_t addr, size_t size)
{
    uint32_t end_addr = FLASH_BASE + LL_GetFlashSize() * 1024;
    if ((addr + size) > end_addr) {
        print_error("ERROR: erase outrange flash size! addr is (0x%p)", (void *)(addr + size));
        return -(ERROR_CODE_INVALID_PARAM);
    }

    end_addr = addr + size;
    end_addr = ALIGN_UP(end_addr, FLASH_PAGE_SIZE);
    addr = ALIGN_DOWN(addr, FLASH_PAGE_SIZE);
    
    int32_t ret = 0;
    uint32_t PAGEError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = addr;
    EraseInitStruct.NbPages = (end_addr - addr) / FLASH_PAGE_SIZE;
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK) {
        ret = -(ERROR_CODE_FAILED);
    }
    HAL_FLASH_Lock();
    print_debug("erase done: addr (0x%p), size %d", (void *)addr, size);
    return ret;
}

// @brief 获取擦除单位大小
uint32_t htd_inflash_erase_unit_get(uint32_t addr)
{
    return FLASH_PAGE_SIZE;
}


