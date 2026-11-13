#define DEBUG_LVL       1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "dev.info"
#include "htd_device.h"
#include "htd_dev_info.h"


#if !defined(_WIN32)
//////////////////////////////////////////////////////////////////
// @brief 获取时钟频率(单位HZ)
__WEAK uint32_t htd_rcc_freq_hz_get(void)
{
    return 72*1000*1000;
}

// @brief 获取芯片id
__WEAK uint32_t htd_mcu_id_get(uint8_t* p_id, uint32_t id_len)
{
    uint8_t* p_start = p_id;
    uint8_t val = 'a';
    while (id_len--) {
        *p_id++ = val++;
    }
    return p_id - p_start;
}

// @brief 获取 stack 起始地址
__WEAK uintptr_t htd_stack_start_addr_get(void)
{
#if defined(HTD_STACK_BASE)
    return HTD_STACK_BASE;
#else
    print_error("stack_start not defined.");
    ASSERT(0);
    return 0;
#endif
}

// @brief 获取 stack 大小
__WEAK size_t htd_stack_size_get(void)
{
#if defined(HTD_STACK_END)
    return HTD_STACK_END - htd_stack_start_addr_get();
#else
    print_error("stack_size not defined.");
    ASSERT(0);
    return 0;
#endif
}

// @brief 获取内存堆信息，返回总共多少个内存堆
// @param idx 第几块内存，partition_count 获取多少块内存信息
__WEAK uint32_t htd_heap_partition_get(htd_ram_partition_t* p_info, uint32_t partition_count, uint32_t idx)
{
    return 0;
}
// @brief 获取 code 起始地址
__WEAK uintptr_t htd_code_start_addr_get(void)
{
    return HTD_CODE_START;
}
// @brief 获取 code 大小, 单位byte
__WEAK size_t htd_code_size_get(void)
{
    return HTD_CODE_SIZE;
}

// @brief 显示设备信息
__WEAK void htd_dev_info_show(void)
{
}

// @brief 显示内存信息
void htd_dev_ram_info_show(void)
{
#if defined(DEBUG_LVL) && (DEBUG_LVL == 4)
    uint32_t idx = 0;
    htd_ram_partition_t ram_partition;
    while (1) {
        if (0 == htd_heap_partition_get(&ram_partition, 1, idx++)) {
            break;
        }
        print_debug("heap%d_start=0x%08x, size=0x%08x", idx, ram_partition.addr, ram_partition.size);
    }
#endif
}

//////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && (!defined(_WIN32)) && 1)
#include "htd_console_process.h"

// @brief 寄存器数值显示
static int register_hexdump_cmd(int argc, char **argv)
{
    if (argc != 3) {
        print_error("Usage: %s [addr] [size]", argv[1]);
        return -1;
    }
    
    uint32_t addr;
    if (0 != str2int(argv[2], &addr)) {
        print_error("param parser failed. argv[2]=%s", argv[2]);
        return -1;
    }
    uint32_t size;
    if (0 != str2int(argv[3], &size)) {
        print_error("param parser failed. argv[3]=%s", argv[3]);
        return -1;
    }

    uint32_t data_len = 128;
    while (size) {
        if (data_len > size) {
            data_len = size;
        }
        htd_console_hexdump((const uint8_t*)addr, data_len);
        addr += data_len;
        size -= data_len;
        htd_delay_ms(30);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(register_hexdump_cmd, reg_hexdump);

// @brief 调试命令
static int device_info_cmd(int argc, char **argv)
{
    uint8_t cpu_id[12];
    htd_mcu_id_get(cpu_id, sizeof(cpu_id));
    print_hex("MCU ID", cpu_id, sizeof(cpu_id));
    print_debug("CPU freq: %u(HZ)", htd_rcc_freq_hz_get());
    print_debug("stack_start=0x%08x, size=0x%08x", htd_stack_start_addr_get(), htd_stack_size_get());
    print_debug("code_start=0x%08x, size=0x%08x", htd_code_start_addr_get(), htd_code_size_get());

    htd_dev_ram_info_show();
    htd_inflash_info_show();
    htd_dev_info_show();
    return 0;
}
CONSOLE_CMD_EXPORT(device_info_cmd, dev_info);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/
#endif /*_WIN32*/

