#define DEBUG_LVL           4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "dev.usbd"
#include "htd_device.h"
#include "htd_dev_usbd.h"


// @brief 销毁句柄
static void htd_usbd_destroy_handle(htd_usbd_handle_t* p_handle)
{
    if (p_handle) {
        htd_free(p_handle);
    }
}

// @brief 打开
htd_usbd_handle_t* htd_usbd_open(const char* p_name, void* p_param)
{
    htd_dev_t* p_dev = htd_dev_find(p_name, HTD_USBD_DEV_IDENTIFY);
    if (p_dev == NULL) {
        print_error("find device failed. %s", p_name);
        return NULL;
    }

    htd_usbd_handle_t* p_handle = htd_malloc(sizeof(htd_usbd_handle_t));
    ASSERT(p_handle);
    htd_memset(p_handle, 0, sizeof(htd_usbd_handle_t));
    p_handle->p_dev = p_dev;
    p_handle->destroy_handle = htd_usbd_destroy_handle;
    
    int32_t ret = p_dev->ctrl_handle(p_handle, HTD_USBD_CMD_OPEN, p_param, 0);
    if (ret != ERROR_CODE_SUCCESS) {
        print_warning("open failed. ret=%d", ret);
        htd_free(p_handle);
        return NULL;
    }
    return p_handle;
}

// @brief 关闭
int32_t htd_usbd_close(htd_usbd_handle_t *p_handle)
{
    ASSERT(p_handle && p_handle->p_dev && p_handle->p_dev->ctrl_handle);
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_USBD_CMD_CLOSE, NULL, 0);
}

// @brief 发送数据
int32_t htd_usbd_write(htd_usbd_handle_t* p_handle, const uint8_t* p_data, uint32_t data_len, uint32_t timeout_ms)
{
    ASSERT(p_handle && p_handle->p_dev && p_handle->p_dev->ctrl_handle);
    htd_usbd_xfer_t xfer;
    xfer.timeout_ms = timeout_ms;
    xfer.p_data = (uint8_t*)p_data;
    xfer.data_len = data_len;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_USBD_CMD_WRITE, &xfer, sizeof(htd_usbd_xfer_t));
}

// @brief 读取数据
int32_t htd_usbd_read(htd_usbd_handle_t* p_handle, uint8_t* p_data, uint32_t data_len, uint32_t timeout_ms)
{
    ASSERT(p_handle && p_handle->p_dev && p_handle->p_dev->ctrl_handle);
    htd_usbd_xfer_t xfer;
    xfer.timeout_ms = timeout_ms;
    xfer.p_data = p_data;
    xfer.data_len = data_len;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_USBD_CMD_READ, &xfer, sizeof(htd_usbd_xfer_t));
}



//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4))
#include "htd_console_process.h"

#if defined(BSP_USING_USBD_CDC_ACM)
static htd_usbd_handle_t* m_usbd_test_handle = NULL;
static volatile uint32_t m_usbd_rx_notify_ms = 0;


// @brief 
static void usbd_vcom_recv_handle(void* arg)
{
    uint8_t data[128];      // 透传时，这个值越大，一次读取的数据越多，发送次数就越少
    uint32_t data_len;
    htd_usbd_handle_t* p_handle = (htd_usbd_handle_t*)arg;
    m_usbd_rx_notify_ms = 0;

    uint32_t write_len = 0;
    while (1) {
        htd_memset(data, 0, sizeof(data));
        data_len = htd_usbd_read(p_handle, data, sizeof(data), 5);
        if (data_len == 0) {
            break;
        }
        print_hex("rx", data, data_len);
        htd_usbd_write(p_handle, data, data_len, 10);
        write_len += data_len;
    }
    //print_debug("write to fifo. len=%d", write_len);
}

// @brief 中断接收到数据了
static int32_t usbd_vcom_evt_notify_handle(htd_usbd_handle_t* p_handle, htd_usbd_evt_t evt, uint8_t* p_param, uint32_t param_len, bool from_irq)
{
    if (evt == HTD_USBD_EVT_RX) {
        if (m_usbd_rx_notify_ms == 0) {
            if (htd_task_load_function_fisr(usbd_vcom_recv_handle, (void*)p_handle)) {
                m_usbd_rx_notify_ms = htd_time_ms_get_fisr();
            }
        }
    }
    return 0;
}

// @brief 调试命令
static int usbd_vcom_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s open/close", argv[0]);
        return 0;
    }
    
    if (0 == htd_memcmp(argv[1], "open", 5)) {
        if (argc != 3) {
            print_error("Usage: %s open [cdc_acm0/cdc_acm1...]", argv[0]);
            return 0;
        }
        if (m_usbd_test_handle) {
            print_error("usbd is opened.");
            return 0;
        }

        htd_usbd_cdc_acm_param_t param = {
            .baudrate = 115200,
            .data_bits = HTD_DATA_BITS_8,
            .stop_bits = HTD_STOP_BITS_1,
            .parity = HTD_PARI_NONE,
            .wfifo_buffer_len = 256,
            .rfifo_buffer_len = 256,
            .evt_notify_handle = usbd_vcom_evt_notify_handle,
        };
        htd_usbd_handle_t* p_handle = htd_usbd_open(argv[2], &param);
        ASSERT(p_handle);
        m_usbd_test_handle = p_handle;
        print_warning("open %s, success.", argv[2]);
        return 0;
    } 
    
    // 关闭
    if (0 == htd_memcmp(argv[1], "close", 6)) {
        if (m_usbd_test_handle) {
            htd_usbd_close(m_usbd_test_handle);
            m_usbd_test_handle = NULL;
        }
        return 0;
    } 

    print_error("Usage: %s open/close", argv[0]);
    return 0;
}
CONSOLE_CMD_EXPORT(usbd_vcom_cmd, vcom);
#endif /*defined(BSP_USING_USBD_CDC_ACM)*/


//////////////////////////////////////////////////////////////////////
#if defined(BSP_USING_USBD_DFU)
static htd_usbd_handle_t* m_usbd_dfu_handle = NULL;

// @brief 中断接收到数据了
static int32_t usbd_dfu_evt_notify_handle(htd_usbd_handle_t* p_handle, htd_usbd_evt_t evt, uint8_t* p_param, uint32_t param_len, bool from_irq)
{
    print_debug("evt=%d, param_len=%d", evt, param_len);
    return 0;
}

// @brief 
// dfu-util -a 0 -D firmware.bin
// -a 指定要操作的 alt 编号
// -D 将指定的固件下载到设备中
// -s 指定固件烧录的起始内存地址。如果你的DFU实现支持DfuSe(ST 公司扩展的 DFU 协议)，可能需要这个参数。例如，-s 0x08000000 表示从地址 0x08000000 开始烧录
// -R 在完成固件下载后，自动复位设备
// -d 当电脑连接了多个 DFU 设备时，用这个参数指定目标设备的 VID 和 PID，避免操作错误
static int32_t usbd_dfu_cmd(int argc, char** argv)
{
    if (argc < 2) {
        print_error("Usage: %s open/close", argv[0]);
        return 0;
    }
    if (0 == htd_memcmp(argv[1], "open", 5)) {
        if (m_usbd_dfu_handle) {
            print_error("usbd dfu has opened.");
            return 0;
        }
        htd_usbd_dfu_param_t param = {.evt_notify_handle = usbd_dfu_evt_notify_handle};
        htd_usbd_handle_t* p_handle = htd_usbd_open("usbd_dfu", &param);
        ASSERT(p_handle);
        m_usbd_dfu_handle = p_handle;
        print_warning("open usbd_dfu success.");
        return 0;
    }
    if (0 == htd_memcmp(argv[1], "close", 6)) {
        if (m_usbd_dfu_handle) {
            htd_usbd_close(m_usbd_dfu_handle);
            m_usbd_dfu_handle = NULL;
        }
        return 0;
    }
    print_error("Usage: %s open/close", argv[0]);
    return 0;
}
CONSOLE_CMD_EXPORT(usbd_dfu_cmd, usbd_dfu);
#endif /*defined(BSP_USING_USBD_DFU)*/


//////////////////////////////////////////////////////////////////////
#if defined(BSP_USING_USBD_MSC) && (BSP_USBD_MSC_DEV_COUNT == 2)
#define USBD_SDCARD_BLOCK_OFFSET        (48 * (2*1024*1024))
#define USBD_SDCARD_BLOCK_COUNT         (8 * (2*1024))
static htd_usbd_handle_t* m_usbd_msc_handle = NULL;
static sdcard_handle_t* m_sdcard_handle = NULL;
static bootuf2_data_t* m_bootuf2 = NULL;


// @brief 中断接收到数据了
static int32_t usbd_msc_evt_notify_handle(htd_usbd_handle_t* p_handle, htd_usbd_evt_t evt, uint8_t* p_param, uint32_t param_len, bool from_irq)
{
    uint32_t block_idx;
    htd_usbd_msc_xfer_t* p_xfer = (htd_usbd_msc_xfer_t*)p_param;
    if (p_xfer->idx == 0) {
        if (evt == HTD_USBD_EVT_MSC_READ) {
            bootuf2_read_sector(m_bootuf2, p_xfer->sector, p_xfer->p_data, p_xfer->sector_count);
        } else if (evt == HTD_USBD_EVT_MSC_WRITE) {
            bootuf2_write_sector(m_bootuf2, p_xfer->sector, p_xfer->p_data, p_xfer->sector_count);
            if (bootuf2_is_write_done(m_bootuf2)) {
                print_warning("transfer finish.");
            }
        }
        return 0;
    }
    
    block_idx = (USBD_SDCARD_BLOCK_OFFSET + (p_xfer->idx-1) * USBD_SDCARD_BLOCK_COUNT) + p_xfer->sector;
    if (evt == HTD_USBD_EVT_MSC_READ) {
        //print_debug("read: sector_count=%d, sector=%d", p_xfer->sector_count, p_xfer->sector);
        htd_dev_sdcard_block_read(m_sdcard_handle, block_idx, p_xfer->p_data, p_xfer->sector_count);
    } else if (evt == HTD_USBD_EVT_MSC_WRITE) {
        //print_debug("write: sector_count=%d, sector=%d", p_xfer->sector_count, p_xfer->sector);
        htd_dev_sdcard_block_write(m_sdcard_handle, block_idx, p_xfer->p_data, p_xfer->sector_count);
    }
    return 0;
}

// @brief 
int bootuf2_norflash_write(uint32_t address, const uint8_t *data, size_t size)
{
    print_warning("address=0x%08x, size=%d", address, size);
    return 0;
}

// @brief 打开sd卡
static int32_t usbd_msc_sdcard_open_handle(const char* p_name)
{
    if (m_usbd_msc_handle || m_sdcard_handle) {
        print_error("usbd dfu has opened.");
        return 0;
    }
    if (!htd_dev_sdcard_is_plug_in()) {
        print_debug("sdcard is not plug in");
        return -1;
    }
    
    m_bootuf2 = (bootuf2_data_t*)htd_malloc(sizeof(bootuf2_data_t));
    ASSERT(m_bootuf2);
    htd_memset(m_bootuf2, 0, sizeof(bootuf2_data_t));
    bootuf2_init(m_bootuf2, bootuf2_norflash_write);
    print_debug("SectorsOver32MB=%d, Sectors=%d", m_bootuf2->DBR->BPB.SectorsOver32MB, m_bootuf2->DBR->BPB.Sectors);
    
    m_sdcard_handle = htd_dev_sdcard_open();
    if (m_sdcard_handle == NULL) {
        return -1;
    }
    dev_sdcard_info_t sdcard_info;
    htd_dev_sdcard_info_get(m_sdcard_handle, &sdcard_info);
    print_debug("sdcard: block_count=%d, block_size=%d", sdcard_info.block_count, sdcard_info.block_size);
    
    htd_usbd_msc_param_t msc_param = {.evt_notify_handle = usbd_msc_evt_notify_handle};
    msc_param.block_count[0] = bootuf2_get_sector_count(m_bootuf2);
    msc_param.block_size[0] = bootuf2_get_sector_size(m_bootuf2);
    msc_param.block_count[1] = USBD_SDCARD_BLOCK_COUNT;
    msc_param.block_size[1] = sdcard_info.block_size;
    
    htd_usbd_handle_t* p_handle = htd_usbd_open(p_name, &msc_param);
    ASSERT(p_handle);
    m_usbd_msc_handle = p_handle;
    print_warning("msc open success.");
    return 0;
}

// @brief 
static int32_t usbd_msc_cmd(int argc, char** argv)
{
    if (argc < 2) {
        print_error("Usage: %s open/close", argv[0]);
        return 0;
    }
    if (0 == htd_memcmp(argv[1], "open", 5)) {
        usbd_msc_sdcard_open_handle("msc");
        return 0;
    }
    
    if (0 == htd_memcmp(argv[1], "close", 6)) {
        if (m_usbd_msc_handle) {
            if (m_sdcard_handle) {
                htd_dev_sdcard_close(m_sdcard_handle);
                m_sdcard_handle = NULL;
            }
            if (m_bootuf2) {
                htd_free(m_bootuf2);
                m_bootuf2 = NULL;
            }
            htd_usbd_close(m_usbd_msc_handle);
            m_usbd_msc_handle = NULL;
        }
        return 0;
    }
    print_error("Usage: %s open/close", argv[0]);
    return 0;
}
CONSOLE_CMD_EXPORT(usbd_msc_cmd, msc);
#endif /*defined(BSP_USING_USBD_MSC)*/


//////////////////////////////////////////////////////////////////////
#if defined(BSP_USING_USBD_MTP)








#endif /*defined(BSP_USING_USBD_MTP)*/
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/



