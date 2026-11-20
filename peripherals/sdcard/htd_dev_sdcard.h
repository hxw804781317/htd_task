#ifndef _HTD_DEV_SDCARD_H_
#define _HTD_DEV_SDCARD_H_

#include <stdint.h>
#include <stdbool.h>
#include "htd_task.h"
#include "htd_dev_sdio.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
typedef void* sdcard_handle_t;


// @brief 初始化
int32_t htd_dev_sdcard_init(void);
// @brief 判断是否已经插入
bool htd_dev_sdcard_is_plug_in(void);
// @brief 打开
sdcard_handle_t htd_dev_sdcard_open(void);
// @brief 关闭 
int32_t htd_dev_sdcard_close(sdcard_handle_t p_handle);
// @brief 读块数据
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdcard_block_read(sdcard_handle_t p_handle, uint32_t block_idx, uint8_t* p_data, uint32_t block_count);
// @brief 写块数据
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdcard_block_write(sdcard_handle_t p_handle, uint32_t block_idx, const uint8_t* p_data, uint32_t block_count);
// @brief 擦除
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdcard_block_erase(sdcard_handle_t p_handle, uint32_t block_idx, uint32_t block_count);
// @brief 获取SD卡信息
int32_t htd_dev_sdcard_info_get(sdcard_handle_t p_handle, dev_sdcard_info_t* p_info);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_SDCARD_H_*/

