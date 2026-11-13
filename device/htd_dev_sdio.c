#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "dev.sdio"
#include "htd_device.h"
#include "htd_dev_sdio.h"
#include "htd_components.h"



//////////////////////////////////////////////////////////////////////
// @brief 打开
sdio_handle_t* htd_dev_sdio_open(const char* p_name, const sdio_user_config_t* p_user_config)
{
    const htd_dev_t* p_dev = htd_dev_find(p_name, HTD_SDIO_DEV_IDENTIFY);
    if (p_dev == NULL) {
        print_error("%s open failed.", p_name);
        return NULL;
    }
    
    // 创建新的设备
    sdio_handle_t* p_handle = htd_malloc(sizeof(sdio_handle_t));
    ASSERT(p_handle);
    htd_memset(p_handle, 0, sizeof(sdio_handle_t));
    p_handle->p_dev = p_dev;
    p_handle->pin_cmd = p_user_config->pin_cmd;
    p_handle->pin_sck = p_user_config->pin_sck;
    p_handle->pin_d0 = p_user_config->pin_d0;
    p_handle->pin_d1 = p_user_config->pin_d1;
    p_handle->pin_d2 = p_user_config->pin_d2;
    p_handle->pin_d3 = p_user_config->pin_d3;
    
    // 打开驱动设备
    int32_t ret = p_dev->ctrl_handle(p_handle, HTD_SDIO_CMD_OPEN, NULL, 0);
    if (ret != ERROR_CODE_SUCCESS) {
        htd_free(p_handle);
        return NULL;
    }
    return p_handle;
}

// @brief 关闭 
int32_t htd_dev_sdio_close(sdio_handle_t* p_handle)
{
    if (p_handle) {
        const htd_dev_t* p_dev = p_handle->p_dev;
        if (p_dev) {
            if (ERROR_CODE_SUCCESS == p_dev->ctrl_handle(p_handle, HTD_SDIO_CMD_CLOSE, NULL, 0)) {
                if (p_handle->pin_cmd != PIN_NUM_INVALID) {
                    htd_pin_hiz_set(p_handle->pin_cmd);
                }
                if (p_handle->pin_sck != PIN_NUM_INVALID) {
                    htd_pin_hiz_set(p_handle->pin_sck);
                }
                if (p_handle->pin_d0 != PIN_NUM_INVALID) {
                    htd_pin_hiz_set(p_handle->pin_d0);
                }
                if (p_handle->pin_d1 != PIN_NUM_INVALID) {
                    htd_pin_hiz_set(p_handle->pin_d1);
                }
                if (p_handle->pin_d2 != PIN_NUM_INVALID) {
                    htd_pin_hiz_set(p_handle->pin_d2);
                }
                if (p_handle->pin_d3 != PIN_NUM_INVALID) {
                    htd_pin_hiz_set(p_handle->pin_d3);
                }
            }
        }
        htd_free(p_handle);
    }
    return 0;
}

// @brief 读块数据
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdio_block_read(sdio_handle_t* p_handle, uint32_t block_idx, uint8_t* p_data, uint32_t block_count)
{
    ASSERT(p_handle);
    htd_sdio_xfer_t xfer;
    xfer.block_idx = block_idx;
    xfer.p_data = p_data;
    xfer.block_count = block_count;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_SDIO_CMD_READ, &xfer, sizeof(htd_sdio_xfer_t));
}

// @brief 写块数据
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdio_block_write(sdio_handle_t* p_handle, uint32_t block_idx, const uint8_t* p_data, uint32_t block_count)
{
    ASSERT(p_handle);
    htd_sdio_xfer_t xfer;
    xfer.block_idx = block_idx;
    xfer.p_data = (uint8_t*)p_data;
    xfer.block_count = block_count;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_SDIO_CMD_WRITE, &xfer, sizeof(htd_sdio_xfer_t));
}

// @brief 擦除
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdio_block_erase(sdio_handle_t* p_handle, uint32_t block_idx, uint32_t block_count)
{
    ASSERT(p_handle);
    htd_sdio_erase_param_t erase_param;
    erase_param.block_idx = block_idx;
    erase_param.block_count = block_count;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_SDIO_CMD_ERASE, &erase_param, sizeof(htd_sdio_erase_param_t));
}

// @brief 获取SD卡信息
int32_t htd_dev_sdio_info_get(sdio_handle_t* p_handle, dev_sdcard_info_t* p_info)
{
    ASSERT(p_handle && p_info);
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_SDIO_CMD_INFO_GET, p_info, sizeof(dev_sdcard_info_t));
}


