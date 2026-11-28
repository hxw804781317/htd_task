// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BOOT_CONFIG_H_
#define _HTD_BOOT_CONFIG_H_

#include "htd_boot_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 状态
#define BOOT_STATE_NONE                     0
#define BOOT_STATE_DOWNLOAD_ING             1       // 正在下载固件
#define BOOT_STATE_DOWNLOAD_FINISH          2       // 已经完成下载固件
#define BOOT_STATE_SWAP_A2B                 3       // 交换数据状态
#define BOOT_STATE_SWAP                     4       // 交换数据
#define BOOT_STATE_SWAP_FINISH              5       // 交换数据完成
#define BOOT_STATE_IMAGE_OK                 6       // 固件正常
#define BOOT_STATE_IMAGE_ERROR              7       // 固件错误
#define BOOT_STATE_ROLLBACK                 8       // 固件正在回滚
#define BOOT_STATE_ROLLBACK_FINISH          9       // 固件回滚完成
#define BOOT_STATE_DECOMPRESS               10      // 解压
typedef uint8_t boot_state_t;

// 升级方式
#define BOOT_UPDATE_TYPE_NONE               0
#define BOOT_UPDATE_TYPE_FULL               1       // 全量升级，A/B两个区，可以回滚
#define BOOT_UPDATE_TYPE_DIFF               2       // 差分升级，A/B两个区，一大一小，不可以回滚
#define BOOT_UPDATE_TYPE_COMPRESS_QUICKLZ   3       // 使用 quicklz 压缩
#define BOOT_UPDATE_TYPE_COMPRESS_FASTLZ    4       // 使用 fastlz 压缩
typedef uint8_t boot_update_type_t;


typedef struct {
    uint32_t magic;
    boot_state_t state;
    boot_update_type_t update_type;         // 升级方式
    uint32_t offset;
    uint32_t offset1;
    uint32_t start_addr;
    uint32_t size;
} boot_info_t;
#define BOOT_INFO_MAGIC                     0x32999ad3


// 操作对象
typedef struct {
    uint32_t init_flag;                 // 初始化标志
    uint32_t index;
    uint32_t var_waddr;
    uint32_t var_addr;                  // 最后写入的地址
    uint16_t var_len;                   // 最后参数大小
    uint32_t erase_unit;
    uint32_t start;
    uint32_t end;
    const boot_flash_handle_t* p_flash;
}boot_flashvar_t;


// @brief 初始化 boot 配置信息，成功返回 true
bool boot_config_init(void);
// @brief 销毁配置信息
void boot_config_deinit(void);


// @brief 获取升级类型
boot_update_type_t boot_config_update_type_get(void);
// @brief 设置升级类型
void boot_config_update_type_set(boot_update_type_t update_type);

// @brief 获取状态
boot_state_t boot_config_state_get(void);
// @brief 获取偏移量
uint32_t boot_config_offset_get(void);
// @brief 保存进度和状态
int32_t boot_config_offset_state_set(uint32_t offset, boot_state_t state);


// @brief 获取配置信息
void boot_config_get(boot_info_t* p_config);
// @brief 写入配置信息
int32_t boot_config_set(boot_info_t* p_config);



// @brief 应用程序配置固件状态
void boot_state_update_in_application(boot_update_type_t update_type, boot_state_t state);




#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BOOT_CONFIG_H_*/

