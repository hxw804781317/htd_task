/*
 * Copyright (c) 2018 Daven <804781317@qq.com>
 * All rights reserved
 */
#define DEBUG_LVL       2       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "sff"
#include "htd_device.h"
#include "drv_norflash_sff_parser.h"



//////////////////////////////////////////////////////////////////////
// SFDP 头部信息
typedef struct {
    uint32_t magic;                 // 魔数，固定为 0x50444653
    uint8_t minor;                  // SFDP 次版本号
    uint8_t major;                  // SFDP 主版本号
    uint8_t num_param_headers;      // 参数头数量
    uint8_t access_protocol;        // 访问协议
}sfdp_header_t;

// 参数头
typedef struct {
    uint16_t id;                    // 参数 ID 
    uint8_t minor;                  // 参数表次版本号
    uint8_t major;                  // 参数表主版本号
    uint8_t len;                    // 参数表长度
    uint32_t addr;                  // 参数地址
} sfdp_param_header_t;

// 基本参数
typedef struct {
    uint8_t support_3byte_addr;     
    uint8_t support_4byte_addr;     
    uint8_t write_unit;             // 写入单位
    uint8_t erase_cmd;              // 擦除命令
    uint32_t erase_unit;            // 擦除单位(字节)
    uint32_t capacity;              // 整个 flash 大小
} sfdp_basic_param_t;


// @brief 读取sfdp数据
static int32_t sfdp_read(sff_spi_transfer_t transfer_handle, uint32_t addr, uint8_t *p_rbuff, size_t rbuff_len)
{
    uint8_t wbuff[5];
    ASSERT(addr < (1L << 24));
    ASSERT(p_rbuff);
    wbuff[0] = 0x5a;
    wbuff[1] = (addr >> 16) & 0xff;
    wbuff[2] = (addr >> 8) & 0xff;
    wbuff[3] = (addr >> 0) & 0xff;
    wbuff[4] = 0xff;
    return transfer_handle(wbuff, sizeof(wbuff), p_rbuff, rbuff_len);
}

// @brief 读取 sfdp 头部数据
int32_t sfdp_header_read(sff_spi_transfer_t transfer_handle, sfdp_header_t* p_hdr)
{
    int32_t ret;
    ASSERT(p_hdr);
    ret = sfdp_read(transfer_handle, 0, (uint8_t*)p_hdr, sizeof(sfdp_header_t));
    if (ret != sizeof(sfdp_header_t)) {
        print_error("SFDP header read failed. ret=%d", ret);
        return -1;
    }
    if (p_hdr->magic != 0x50444653) {
        print_error("SFDP signature check failed. 0x%08x(0x50444653)", p_hdr->magic);
        return -2;
    }
    print_debug("read SFDP header ok. verson=%d.%d, num_param_headers=%d", 
        p_hdr->major, p_hdr->minor, p_hdr->num_param_headers);
    return 0;
}

// @brief 读取参数头信息
int32_t sfdp_param_header_read(sff_spi_transfer_t transfer_handle, uint32_t addr, sfdp_param_header_t *p_param_hdr) 
{
    ASSERT(p_param_hdr);
    
    uint8_t rbuff[8];
    int32_t ret = sfdp_read(transfer_handle, addr, rbuff, sizeof(rbuff));
    if (ret != sizeof(rbuff)) {
        print_error("sfdp read failed. ret=%d", ret);
        return ret;
    }
    
    p_param_hdr->id = (rbuff[7] << 8) | rbuff[0];
    p_param_hdr->minor = rbuff[1];
    p_param_hdr->major = rbuff[2];
    p_param_hdr->len = rbuff[3];
    p_param_hdr->addr = ((long)rbuff[4]) | ((long)rbuff[5] << 8) | ((long)rbuff[6] << 16);
    if (p_param_hdr->major > 1) {
        print_error("error: This version(V%d.%d) is not supported.", p_param_hdr->major, p_param_hdr->minor);
        return -1;
    }
    if (p_param_hdr->len < 9) {
        print_error("error: The JEDEC basic flash parameter table length (now is %d) error.", p_param_hdr->len);
        return false;
    }
    print_debug("read param header success. id=0x%04x, version=V%d.%d, length=%d, addr=%u",
            p_param_hdr->id, p_param_hdr->major, p_param_hdr->minor, p_param_hdr->len, p_param_hdr->addr);
    return 0;
}

// @brief 读取基础参数
int32_t sfdp_basic_param_read(sff_spi_transfer_t transfer_handle, sfdp_basic_param_t* p_basic_param)
{
    sfdp_param_header_t param_header;
    int32_t ret = sfdp_param_header_read(transfer_handle, 8, &param_header);
    if (ret != 0) {
        return ret;
    }
    
    uint8_t rbuff[36];
    ret = sfdp_read(transfer_handle, param_header.addr, rbuff, sizeof(rbuff));
    if (ret != sizeof(rbuff)) {
        print_error("read basic param failed. ret=%d", ret);
        return ret;
    }
    htd_memset(p_basic_param, 0, sizeof(sfdp_basic_param_t));
    
    // 解析 write unit
    p_basic_param->write_unit = 4;
    if (rbuff[0] & __BIT(2)) {
        p_basic_param->write_unit = 1;
    }
    
    // 解析 capacity 
    uint32_t table2_temp = ((uint32_t)rbuff[7] << 24) | ((uint32_t)rbuff[6] << 16) | ((uint32_t)rbuff[5] << 8) | (uint32_t)rbuff[4];
    if (rbuff[7] & __BIT(7)) {
        table2_temp &= 0x7FFFFFFF;
        if (table2_temp > sizeof(p_basic_param->capacity) * 8 + 3) {
            p_basic_param->capacity = 0;
            print_error("Error: The flash capacity is grater than 32Gb/4GB! Not Supported.");
            return -2;
        }
        p_basic_param->capacity = 1L << (table2_temp - 3);
    } else {
        p_basic_param->capacity = 1L + (table2_temp >> 3);
    }
    print_warning("Capacity is %u(MByte).", p_basic_param->capacity / (1024*1024));
    
    // 解析擦除单位和擦除大小
    uint32_t erase_unit;
    p_basic_param->erase_unit = 0xffffffff;
    uint32_t pos = 28;
    for (uint32_t i=0; i<4; i++) {
        if (rbuff[pos] != 0x00) {
            erase_unit = 1L << rbuff[pos];
            if (p_basic_param->erase_unit > erase_unit) {
                p_basic_param->erase_unit = erase_unit;
                p_basic_param->erase_cmd = rbuff[pos+1];
            }
        } 
        print_debug("type=0x%02x, cmd=0x%02x", rbuff[pos], rbuff[pos+1]);
        pos += 2;
    }
    print_warning("erase_cmd=0x%02x, erase_unit=%u(Byte)", p_basic_param->erase_cmd, p_basic_param->erase_unit);
    return 0;
}

// @brief 读取 jedec id
static int32_t sflash_jedec_id_read(sff_spi_transfer_t transfer_handle, sff_flash_info_t* p_info) 
{
    uint8_t wbuff[1], rbuff[4];
    for (uint32_t i=0; i<3; ++i) {
        wbuff[0] = 0x9f;
        htd_memset(rbuff, 0, sizeof(rbuff));
        int32_t ret = transfer_handle(wbuff, sizeof(wbuff), rbuff, sizeof(rbuff));
        if (ret == sizeof(rbuff)) {
            p_info->mf_id = rbuff[0];
            p_info->type_id = rbuff[1];
            p_info->capacity_id = rbuff[2];
        } else {
            print_error("Error: Read device JEDEC ID error.");
            print_hex("data", rbuff, 3);
            return -1;
        }
        if (p_info->mf_id != 0xff && p_info->mf_id != 0) {
            break;
        }
        htd_delay_ms(5);
    }
    print_debug("mf_id=0x%02x, type_id=0x%02x, capacity_id=0x%02x", p_info->mf_id, p_info->type_id, p_info->capacity_id);
    return 0;
}

// @brief 获取 sff 解析的信息
int32_t sff_parser_info_get(sff_spi_transfer_t transfer_handle, sff_flash_info_t* p_info)
{
    int32_t ret;
    ASSERT(p_info);
    htd_memset(p_info, 0, sizeof(sff_flash_info_t));

    // 读取芯片厂商id
    ret = sflash_jedec_id_read(transfer_handle, p_info);
    if (ret != 0) {
        print_error("jedec id read failed. ret=%d", ret);
        return ret;
    }

    // 获取芯片匹配信息
    const sff_flash_info_t* p_ff = sff_flash_info_get(p_info->mf_id, p_info->type_id, p_info->capacity_id);
    if (p_ff) {
        htd_memcpy(p_info, p_ff, sizeof(sff_flash_info_t));
        return 0;
    }

    // 读取 SFDP 头部信息
    sfdp_header_t sfdp_header;
    ret = sfdp_header_read(transfer_handle, &sfdp_header);
    if (ret != 0) {
        print_error("read sfdp header feiled. ret=%d", ret);
        return ret;
    }
    
    // 读取 basic 基础参数
    sfdp_basic_param_t basic_param;
    ret = sfdp_basic_param_read(transfer_handle, &basic_param);
    if (ret != 0) {
        print_error("read sfdp basic param failed. ret=%d", ret);
        return -1;
    }
    
    // 返回基本信息
    p_info->capacity = basic_param.capacity;
    p_info->erase_unit = basic_param.erase_unit;
    p_info->erase_cmd = basic_param.erase_cmd;
    p_info->write_unit = 4;
    p_info->page_size = 256;
    return 0;
}

