#define DEBUG_LVL               2       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG               "nor.win32"
#include "htd_dev_norflash.h"


#if defined(HTD_VFS_ENABLE) && defined(_WIN32)
#include "htd_vfs_process.h"
//////////////////////////////////////////////////////////////////////
// 模拟一块16M的 norflash
#define NORFLASH_FILE_PATH                  "/norflash.bin"
#define NORFLASH_PROGRAM_COUNT              100000
#define NORFLASH_START_ADDR                 0x12000000
#define NORFLASH_SIZE                       (16*1024*1024)
#define NORFLASH_ERASE_UNIT                 4096
#define NORFLASH_PAGE_SIZE                  4096
static int32_t m_norflash_fd = -1;

typedef struct {
    int32_t program_cnt;
} norflash_page_head_t;

// @brief 打开文件
static int32_t norflash_file_open(void)
{
    if (m_norflash_fd < 0) {
        m_norflash_fd = open(NORFLASH_FILE_PATH, O_RDWR|O_CREAT, 0777);
        if (m_norflash_fd < 0) {
            print_error("open file failed. filepath=%s, fd=%d", NORFLASH_FILE_PATH, m_norflash_fd);
            ASSERT(0);
        }
    }
    return m_norflash_fd;
}

// @brief 关闭文件
static int32_t norflash_file_close(void)
{
#if 1
    if (m_norflash_fd > 0) {
        close(m_norflash_fd);
        m_norflash_fd = -1;
    }
#endif
    return 0;
}

// @brief 判断某一页读写次数是否已经超过预定的次数
static int32_t norflash_program_count_update(int32_t fd, uint32_t addr, bool is_update)
{
    // 将地址重置到页首
    addr -= NORFLASH_START_ADDR;
    int32_t offset = addr / (NORFLASH_ERASE_UNIT) * (NORFLASH_ERASE_UNIT + sizeof(norflash_page_head_t));
    int32_t ret = lseek(fd, offset, SEEK_SET);
    ASSERT(offset == ret);

    norflash_page_head_t page_head;
    htd_memset(&page_head, 0, sizeof(norflash_page_head_t));
    ret = read(fd, &page_head, sizeof(norflash_page_head_t));
    if (ret == sizeof(norflash_page_head_t) && page_head.program_cnt > NORFLASH_PROGRAM_COUNT) {
        // 已经擦写坏了
        print_error("program count: %d", page_head.program_cnt);
        ASSERT(0);
    }
    
    if (is_update) {
        page_head.program_cnt++;
        ret = lseek(fd, offset, SEEK_SET);
        ASSERT(offset == ret);
        write(fd, &page_head, sizeof(norflash_page_head_t));
    }
    return 0;
}

// @brief 重置地址
static uint32_t norflash_addr_reset(uint32_t addr)
{
    uint32_t offset = addr % NORFLASH_ERASE_UNIT;
    addr = addr / NORFLASH_ERASE_UNIT * (NORFLASH_ERASE_UNIT + sizeof(norflash_page_head_t));
    return addr + offset + sizeof(norflash_page_head_t);
}

// @brief 写函数, 只能从1->0, 不能从0->1, 不能跨页写入
static int32_t norflash_simulator_write(uint32_t addr, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(p_data);
    ASSERT(data_len);
    ASSERT(ALIGN_DOWN(addr, NORFLASH_PAGE_SIZE) == ALIGN_DOWN(addr + data_len - 1, NORFLASH_PAGE_SIZE));
    
    int32_t fd = norflash_file_open();
    ASSERT(fd > 0);
    norflash_program_count_update(fd, addr, false);

    uint32_t addr1 = norflash_addr_reset(addr - NORFLASH_START_ADDR);
    uint32_t offset = addr1;
    int32_t ret = lseek(fd, offset, SEEK_SET);
    ASSERT(offset == ret);
    //print_warning("write. addr=0x%x, offset=0x%x, data_len=%d", addr, offset, data_len);

    int32_t i;
    uint8_t buff[256];
    int32_t buff_len = sizeof(buff);
    const uint8_t* p_start = p_data;
    while (data_len) {
        if (buff_len > (int32_t)data_len) {
            buff_len = data_len;
        }
        // 读数据
        htd_memset(buff, 0xff, buff_len);
        ret = read(fd, buff, buff_len);
        ASSERT(ret >= 0);
        // 重新偏移回去
        ret = lseek(fd, offset, SEEK_SET);
        ASSERT(offset == ret);
        
        // 写入，只能从1->0
        for (i=0; i<buff_len; ++i) {
            buff[i] &= p_data[i];
        }
        ret = write(fd, buff, buff_len);
        ASSERT(ret == buff_len);
        offset += buff_len;
        p_data += buff_len;
        data_len -= buff_len;
    }

#if 1
    // 校验写入的数据是否正确
    offset = addr1;
    ret = lseek(fd, offset, SEEK_SET);
    ASSERT(offset == ret);
    data_len = p_data - p_start;
    p_data = p_start;
    buff_len = sizeof(buff);
    while (data_len) {
        if (buff_len > (int32_t)data_len) {
            buff_len = data_len;
        }
        ret = read(fd, buff, buff_len);
        ASSERT(buff_len == ret);
        if (memcmp(p_data, buff, buff_len)) {
            print_error("data write failed. buff_len=%d, compare_len=%d, addr1=0x%08x", buff_len, p_data-p_start, addr1);
            ASSERT(0);
        }
        p_data += buff_len;
        data_len -= buff_len;
    }
#endif
    
    norflash_file_close();
    return p_data - p_start;
}

// @brief 读函数, 不能跨页读取
static int32_t norflash_simulator_read(uint32_t addr, uint8_t* p_data, uint32_t data_len)
{
    ASSERT(p_data);
    ASSERT(data_len);
    ASSERT(ALIGN_DOWN(addr, NORFLASH_PAGE_SIZE) == ALIGN_DOWN(addr + data_len - 1, NORFLASH_PAGE_SIZE));
    
    int32_t fd = norflash_file_open();
    ASSERT(fd > 0);
    
    uint32_t offset = norflash_addr_reset(addr - NORFLASH_START_ADDR);
    //print_warning("read. addr=0x%x, offset=0x%x, data_len=%d", addr, offset, data_len);
    int32_t ret = lseek(fd, offset, SEEK_SET);
    //print_debug("addr=0x%08x, offset=%d, ret=%d", addr, offset, ret);
    ASSERT(offset == ret);

    htd_memset(p_data, 0xff, data_len);
    ret = read(fd, p_data, data_len);
    ASSERT(ret >= 0);
    norflash_file_close();
    return ret;
}

// @brief 擦除函数
static int32_t norflash_simulator_erase(uint32_t addr)
{
    ASSERT(0 == (addr & (NORFLASH_ERASE_UNIT - 1)));

    uint32_t size = NORFLASH_ERASE_UNIT;
    int32_t fd = norflash_file_open();
    ASSERT(fd > 0);
    
    norflash_program_count_update(fd, addr, true);
    
    uint32_t offset = norflash_addr_reset(addr - NORFLASH_START_ADDR);
    print_warning("erase. addr=0x%x, offset=0x%x", addr, offset);
    int32_t ret = lseek(fd, offset, SEEK_SET);
    ASSERT(offset == ret);

    uint8_t buff[256];
    uint32_t data_len = sizeof(buff);
    htd_memset(buff, 0xff, data_len);
    while (size) {
        if (data_len > size) {
            data_len = size;
        }
        ret = write(fd, buff, data_len);
        ASSERT(ret == data_len);
        size -= data_len;
    }
    norflash_file_close();
    return 0;
}

// @brief 展示每一页的擦除次数
static void norflash_simulator_info_show(void)
{
    norflash_page_head_t page_head;
    int32_t fd = norflash_file_open();
    ASSERT(fd > 0);

    uint32_t total_cnt = 0;
    int32_t max_cnt = 0, min_cnt = 0x7fffffff;
    uint32_t offset = 0;
    while (offset < NORFLASH_SIZE) {
        lseek(fd, offset, SEEK_SET);
        htd_memset(&page_head, 0, sizeof(page_head));
        read(fd, &page_head, sizeof(page_head));
        print_debug("program_cnt: %-6d", page_head.program_cnt);
        offset += sizeof(norflash_page_head_t) + NORFLASH_ERASE_UNIT;
        if (page_head.program_cnt > max_cnt) {
            max_cnt = page_head.program_cnt;
        }
        if (page_head.program_cnt < min_cnt) {
            min_cnt = page_head.program_cnt;
        }
        total_cnt += page_head.program_cnt;
    }
    norflash_file_close();
    print_debug("program: total(%d), max(%d), min(%d)", total_cnt, max_cnt, min_cnt);
}

// @brief 擦除函数
static int32_t norflash_simulator_ctrl(norflash_cmd_t cmd, void* p_param, uint32_t param_len)
{
    if (cmd == NORFLASH_CMD_SECTOR_ERASE) {
        ASSERT(p_param);
        norflash_erase_param_t* p_erase = (norflash_erase_param_t*)p_param;
        uint32_t page_size = 0;
        uint32_t addr = p_erase->addr;
        while (page_size < p_erase->size) {
            norflash_simulator_erase(addr);
            page_size += NORFLASH_ERASE_UNIT;
            addr += NORFLASH_ERASE_UNIT;
        }
    } else if (cmd == NORFLASH_CMD_ADDR_GET) {
        ASSERT(p_param);
        norflash_addr_param_t* p_info = (norflash_addr_param_t*)p_param;
        p_info->phy_saddr = NORFLASH_START_ADDR;
        p_info->phy_eaddr = NORFLASH_START_ADDR + NORFLASH_SIZE;
        return sizeof(norflash_addr_param_t);
    } else if (cmd == NORFLASH_CMD_SIZE_GET) {
        ASSERT(p_param);
        *(uint32_t*)p_param = NORFLASH_SIZE;
        return sizeof(uint32_t);
    } else if (cmd == NORFLASH_CMD_SHOW_INFO) {
        norflash_simulator_info_show();
    }
    return 0;
}

// @brief 初始化, 创建一个对等大小的文件
static int32_t norflash_simulator_init(void)
{
    uint32_t flash_size = NORFLASH_SIZE / NORFLASH_ERASE_UNIT * (NORFLASH_ERASE_UNIT + sizeof(norflash_page_head_t));
    int32_t fd = norflash_file_open();
    ASSERT(fd > 0);

    // 已经创建好了，不用再创建
    int32_t ret = lseek(fd, 0, SEEK_END);
    if (ret == flash_size) {
        norflash_file_close();
        return 0;
    }

    // 创建虚拟norflash, 全部写入0xff
    uint8_t buff[256];
    uint32_t size = flash_size;
    uint32_t data_len = sizeof(buff);
    htd_memset(buff, 0xff, data_len);
    while (size) {
        if (data_len > size) {
            data_len = size;
        }
        ret = write(fd, buff, data_len);
        ASSERT(ret == data_len);
        size -= data_len;
    }
    norflash_file_close();
    return 0;
}

// @brief 注册到上层应用中
static int32_t norflash_simulator_register(void)
{
    static const dev_norflash_t norflash_simulator = {
        .p_name = "simulator",
        .phy_saddr = NORFLASH_START_ADDR,
        .erase_unit = NORFLASH_ERASE_UNIT,
        .page_size = NORFLASH_PAGE_SIZE,
        .write_handle = norflash_simulator_write,
        .read_handle = norflash_simulator_read,
        .ctrl_handle = norflash_simulator_ctrl,
        .init_handle = norflash_simulator_init,
    };
    print_debug("norflash simulator register.");
    return htd_dev_norflash_register(&norflash_simulator);
}
DEVICE_INIT_FUNC_REGISTER(norflash_simulator_register);

#endif /*defined(HTD_VFS_ENABLE) && defined(_WIN32)*/

