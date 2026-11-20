#ifndef _HTD_DEV_NANDFLASH_H_
#define _HTD_DEV_NANDFLASH_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
#define NANDFLASH_ERR_BAD_BLOCK         -1234

#define NANDFLASH_CMD_NONE              0
#define NANDFLASH_CMD_ERASE             1
#define NANDFLASH_CMD_INFO_SHOW         2
typedef uint32_t nandflash_cmd_t;




//////////////////////////////////////////////////////////////////////
typedef struct {
    uint32_t phy_saddr;
    uint16_t page_data_size;            // 页数据大小, 单位字节
    uint16_t page_spare_size;           // 页附加大小
    uint16_t ecc_size;                  // ecc 占用的大小
    uint16_t block_size;                // 块大小，单位页
    uint16_t plane_size;                // 片大小，单位块
    uint16_t plane_count;               // 片数量
    // 写一页数据
    int32_t (*page_write_handle)(uint32_t page_id, 
                                 const uint8_t* p_page_data, uint32_t page_data_len, 
                                 const uint8_t* p_spare, uint32_t spare_len);
    // 读一页数据
    int32_t (*page_read_handle)(uint32_t page_id, 
                                uint32_t offset, uint8_t* p_page_data, uint32_t page_data_len, 
                                uint8_t* p_spare, uint32_t spare_len);
    // ECC生成
    uint32_t (*ecc_generate_handle)(const uint8_t* p_data, uint32_t data_len, uint8_t *ecc);
    // ECC验证
    //int32_t (*ecc_verify_handle)(const uint8_t* p_data, uint32_t data_len, const uint8_t *ecc);
    // ECC修复; 返回0,数据正确不用修复返回0，修复失败返回-1，数据修复成功返回1
    int32_t (*ecc_repair_handle)(uint8_t* p_data, uint32_t data_len, const uint8_t* ecc);
    // 控制句柄
    int32_t (*ctrl_handle)(nandflash_cmd_t cmd, void* p_param, uint32_t param_len);
    // 初始化
    int32_t (*init_handle)(void);
} dev_nandflash_t;




//////////////////////////////////////////////////////////////////////
// @brief 注册(只能调用一次)
int32_t htd_dev_nandflash_register(const dev_nandflash_t* p_nandflash);
// @brief 按页读数据(page_id 为页序号)
int32_t htd_dev_nandflash_page_read(uint32_t page_id, uint32_t offset, uint8_t* p_data, uint32_t data_len);
// @brief 按页写数据
int32_t htd_dev_nandflash_page_write(uint32_t page_id, const uint8_t* p_data, uint32_t data_len);
// @brief 擦除一块数据
int32_t htd_dev_nandflash_block_erase(uint32_t block_id);
// @brief 展示已经挂载的nandflash信息
void htd_dev_nandflash_show(void);
// @brief 获取页数据内容大小
uint32_t htd_dev_nandflash_page_data_size_get(void);
// @brief 获取spare大小
uint32_t htd_dev_nandflash_page_spare_size_get(void);
// @brief 获取ecc大小
uint32_t htd_dev_nandflash_ecc_size_get(void);
// @brief 获取一个块多少页
uint32_t htd_dev_nandflash_pages_per_block_get(void);
// @brief 获取总共多少块
uint32_t htd_dev_nandflash_block_count_get(void);
// @brief 初始化
int32_t htd_dev_nandflash_init(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_NANDFLASH_H_*/

