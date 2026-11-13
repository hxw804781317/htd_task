#define DEBUG_LVL           1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "dev.inflash"
#include "htd_device.h"
#include "htd_dev_inflash.h"



//////////////////////////////////////////////////////////////////////
// @brief 获取flash信息，返回总共多少个内存堆
// @param idx 第几块flash，partition_count 获取多少块内存信息
__WEAK uint32_t htd_inflash_partition_get(htd_flash_partition_t* p_info, uint32_t partition_count, uint32_t idx)
{
    return 0;
}

// @brief 判断是否为内部flash块
__WEAK bool htd_inflash_bank_is(uint32_t start_addr, uint32_t end_addr)
{
    return false;
}

// @brief 读取片内flash数据
// @return 返回读取到的数据长度
__WEAK int32_t htd_inflash_read(uint32_t addr, uint8_t *p_buff, size_t buff_len)
{
    return 0;
}

// @brief 写入片内flash数据
// @return 返回写入的数据长度
__WEAK int32_t htd_inflash_write(uint32_t addr, const uint8_t *p_buff, size_t buff_len)
{
    return 0;
}

// @brief 擦除片内flash
__WEAK int32_t htd_inflash_erase(uint32_t addr, size_t size)
{
    return 0;
}

// @brief 获取擦除单位大小
__WEAK uint32_t htd_inflash_erase_unit_get(uint32_t addr)
{
    return 4096;
}


// @brief 信息显示
void htd_inflash_info_show(void)
{
#if defined(DEBUG_LVL) && (DEBUG_LVL == 4)
    uint32_t idx = 0;
    htd_flash_partition_t partition;
    while (1) {
        if (htd_inflash_partition_get(&partition, 1, idx++) == 0) {
            break;
        }
        print_debug("inflash partition[%02d]: addr=0x%08x, size=0x%08x, unit=%d(KB)", 
                        idx, partition.addr, partition.size, partition.erase_unit/1024);
    }
#endif
}


//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && (!defined(_WIN32)))
#include "htd_console_process.h"

// @brief 获取代码后面第一页的地址
static uint32_t rom_code_next_page_addr_get(uint32_t* erase_unit)
{
    uint32_t saddr, n;
    uint32_t idx = 0;
    htd_flash_partition_t partition;
    uint32_t code_end_addr = htd_code_start_addr_get() + htd_code_size_get();
    while (1) {
        if (htd_inflash_partition_get(&partition, 1, idx++) == 0) {
            break;
        }
        n = partition.size / partition.erase_unit;
        saddr = partition.addr;
        while (n--) {
            if (code_end_addr >= saddr && code_end_addr < saddr + partition.erase_unit) {
                if (erase_unit) {
                    *erase_unit = partition.erase_unit;
                }
                saddr += partition.erase_unit;      // 返回下一页起始地址
                return saddr;
            } 
            saddr += partition.erase_unit;
        }
    }
    return 0;
}

// @brief 获取最终地址
static uint32_t rom_end_addr_get(void)
{
    uint32_t idx = 0;
    htd_flash_partition_t partition;
    uint32_t end_addr = 0;
    while (1) {
        if (htd_inflash_partition_get(&partition, 1, idx++) == 0) {
            break;
        }
        end_addr = partition.addr + partition.size - 1;
    }
    return end_addr;
}

// @brief 测试; 代码后面的两个sector进行读写操作
static void inflash_test_handle(void)
{
    uint32_t erase_unit = 0;
    uint32_t addr = rom_code_next_page_addr_get(&erase_unit);
    if (addr == 0) {
        print_warning("not inflash space to test.");
        return;
    }

    // 代码后面至少预留两页才能进行后面的读写擦测试
    uint32_t end_addr = rom_end_addr_get();
    if (addr + erase_unit*2 > end_addr + 1) {
        print_warning("not inflash space to test.");
        return;
    }
    
    // 擦除
    print_debug("erase. addr=0x%08x. erase_unit=%d", addr, erase_unit);
    htd_inflash_erase(addr, erase_unit*2);
    
    // 写入
    uint8_t buff[128];
    const uint8_t write_byte = 0x81;
    uint32_t buff_len = sizeof(buff);
    uint32_t len = erase_unit + sizeof(buff);
    htd_memset(buff, write_byte, sizeof(buff));
    addr += erase_unit / 2;
    uint32_t saddr = addr;
    while (len) {
        if (buff_len > len) {
            buff_len = len;
        }
        htd_inflash_write(saddr, buff, buff_len);
        saddr += buff_len;
        len -= buff_len;
    }
    print_debug("write finish.");

    // 读取验证
    buff_len = sizeof(buff);
    len = erase_unit + sizeof(buff);
    saddr = addr;
    while (len) {
        if (buff_len > len) {
            buff_len = len;
        }
        htd_memset(buff, 0, buff_len);
        htd_inflash_read(saddr, buff, buff_len);
        if (htd_mem_allof(buff, write_byte, buff_len) == false) {
            print_error("inflash test failed.");
            return ;
        }
        saddr += buff_len;
        len -= buff_len;
    }
    print_warning("inflash test success.");
}

// @brief 展示某个区域的数值
static void inflash_data_dump(uint32_t addr, uint32_t size)
{
    uint8_t data[128];
    uint32_t data_len = sizeof(data);
    while (size) {
        if (data_len > size) {
            data_len = size;
        }
        htd_inflash_read(addr, data, data_len);
        htd_console_hexdump(data, data_len);
        addr += data_len;
        size -= data_len;
        htd_delay_ms(30);
    }
}

// @brief 调试命令
static int inflash_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s show/test", argv[0]);
        return 0;
    }
    if (0 == memcmp(argv[1], "show", 5)) {
        htd_inflash_info_show();
        return 0;
    }
    if (0 == memcmp(argv[1], "test", 5)) {
        inflash_test_handle();
        return 0;
    }
    if (0 == memcmp(argv[1], "dump", 5)) {
        if (argc != 4) {
            print_error("Usage: %s dump [addr] [size]");
            return -1;
        }
        uint32_t addr, size;
        if (0 != str2int(argv[2], &addr)) {
            print_error("param parser failed. argv[2]=%s", argv[2]);
             return -1;
        }
        if (0 != str2int(argv[3], &size)) {
            print_error("param parser failed. argv[3]=%s", argv[3]);
            return -1;
        }
        // 展示某个区域的数据
        print_warning("hexdump: addr=0x%08x, size=%u", addr, size);
        inflash_data_dump(addr, size);
        return 0;
    }
    print_error("Usage: %s show/test", argv[0]);
    return 0;
}
CONSOLE_CMD_EXPORT(inflash_cmd, inflash);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0) && 1)*/

