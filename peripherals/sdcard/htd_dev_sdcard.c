#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "dev.sdcard"
#include "htd_device.h"
#include "htd_dev_sdcard.h"
#include "htd_components.h"


//////////////////////////////////////////////////////////////////////
#if (defined(HTD_SDCARD_DETECT_PIN) && (HTD_SDCARD_DETECT_PIN != PIN_NUM_INVALID))
#define SDCARD_PIN_DEBOUNCE_MAGIC       0xA8
typedef struct {
    uint8_t magic;
    uint8_t pin_level;
    uint16_t count;
}sdcard_pin_debounce_t;

// @brief 是否插入
static bool sdcard_pin_read(void)
{
    uint8_t pin_level = htd_pin_read(HTD_SDCARD_DETECT_PIN);
#ifndef HTD_SDCARD_DETECT_PIN_PLUG_HIGH
    pin_level = !pin_level;
#endif
    return pin_level;
}

// @brief SD卡探测句柄
void sdcard_detect_notify_handle(void* arg)
{
    sdcard_pin_debounce_t pin_debounce;
    uintptr_t param = (uintptr_t)arg;
    htd_memcpy(&pin_debounce, &param, sizeof(sdcard_pin_debounce_t));
    //print_warning("magic=0x%02x, pin_level=%d, count=%d", pin_debounce.magic, pin_debounce.pin_level, pin_debounce.count);
    if (pin_debounce.magic != SDCARD_PIN_DEBOUNCE_MAGIC) {
        return;
    }
    
    uint8_t pin_level = sdcard_pin_read();
    if (pin_debounce.pin_level != pin_level) {
        pin_debounce.count = 0;
        pin_debounce.pin_level = pin_level;
    } else {
        pin_debounce.count++;
    }

    uint8_t plug_in = false;
    if (pin_debounce.count >= 10) {
        if (pin_debounce.pin_level == 1) {
            plug_in = true;
        }
        htd_dev_evt_broadcast(HTD_DEV_EVT_SDCARD_PLUG, &plug_in, sizeof(uint8_t));
        return;
    }
    
    htd_memcpy(&param, &pin_debounce, sizeof(sdcard_pin_debounce_t));
    htd_task_load_timer_function(false, 50, sdcard_detect_notify_handle, (void*)param);
}

// @brief 探测中断句柄
static void sdcard_detect_pin_irq_handle(void)
{
    ASSERT(sizeof(sdcard_pin_debounce_t) <= sizeof(uintptr_t));
    sdcard_pin_debounce_t pin_debounce;
    pin_debounce.magic = SDCARD_PIN_DEBOUNCE_MAGIC;
    pin_debounce.pin_level = htd_pin_read(HTD_SDCARD_DETECT_PIN);
    pin_debounce.count = 0;

    uintptr_t param;
    htd_memcpy(&param, &pin_debounce, sizeof(sdcard_pin_debounce_t));
    htd_task_load_function_fisr(sdcard_detect_notify_handle, (void*)param);
}
HTD_PIN_INT_FUNC_REGISTER(HTD_SDCARD_DETECT_PIN, sdcard_detect_pin_irq_handle);
#endif /*(defined(HTD_SDCARD_DETECT_PIN) && (HTD_SDCARD_DETECT_PIN != PIN_NUM_INVALID))*/





//////////////////////////////////////////////////////////////////////
#if defined(HTD_SDCARD_PIN_CMD) && (HTD_SDCARD_PIN_CMD != PIN_NUM_INVALID)

// @brief 初始化
int32_t htd_dev_sdcard_init(void)
{
#if (defined(HTD_SDCARD_DETECT_PIN) && (HTD_SDCARD_DETECT_PIN != PIN_NUM_INVALID))
    htd_pin_int_set(HTD_SDCARD_DETECT_PIN, HTD_PIN_INT_FALLING, true);
#endif
    return 0;
}

// @brief 判断是否已经插入
bool htd_dev_sdcard_is_plug_in(void)
{
#if (defined(HTD_SDCARD_DETECT_PIN) && (HTD_SDCARD_DETECT_PIN != PIN_NUM_INVALID))
    return sdcard_pin_read();
#else
    return false;
#endif
}

// @brief 打开
sdcard_handle_t htd_dev_sdcard_open(void)
{ 
    sdio_user_config_t sdio_user_config;
    sdio_user_config.pin_cmd = HTD_SDCARD_PIN_CMD;
    sdio_user_config.pin_sck = HTD_SDCARD_PIN_SCK;
    sdio_user_config.pin_d0 = HTD_SDCARD_PIN_D0;
    sdio_user_config.pin_d1 = HTD_SDCARD_PIN_D1;
    sdio_user_config.pin_d2 = HTD_SDCARD_PIN_D2;
    sdio_user_config.pin_d3 = HTD_SDCARD_PIN_D3;
    return (sdcard_handle_t)htd_dev_sdio_open(HTD_SDCARD_BUS_NAME, &sdio_user_config);
}

// @brief 关闭 
int32_t htd_dev_sdcard_close(sdcard_handle_t p_handle)
{
    int32_t ret = htd_dev_sdio_close((sdio_handle_t*)p_handle);
    if (ret != ERROR_CODE_SUCCESS) {
        return ret;
    }
    return ret;
}

// @brief 读块数据
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdcard_block_read(sdcard_handle_t p_handle, uint32_t block_idx, uint8_t* p_data, uint32_t block_count)
{
    return htd_dev_sdio_block_read((sdio_handle_t*)p_handle, block_idx, p_data, block_count);
}

// @brief 写块数据
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdcard_block_write(sdcard_handle_t p_handle, uint32_t block_idx, const uint8_t* p_data, uint32_t block_count)
{
    return htd_dev_sdio_block_write((sdio_handle_t*)p_handle, block_idx, p_data, block_count);
}

// @brief 擦除
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdcard_block_erase(sdcard_handle_t p_handle, uint32_t block_idx, uint32_t block_count)
{
    return htd_dev_sdio_block_erase((sdio_handle_t*)p_handle, block_idx, block_count);
}

// @brief 获取SD卡信息
int32_t htd_dev_sdcard_info_get(sdcard_handle_t p_handle, dev_sdcard_info_t* p_info)
{
    return htd_dev_sdio_info_get((sdio_handle_t*)p_handle, p_info);
}
#endif




//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0) && 0)
#include "htd_console_process.h"

static sdcard_handle_t sdcard_handle = NULL;
const static uint32_t rwdata_len = 1024;
const static uint32_t block_idx = 0;

// @brief 调试命令
static int htd_sdcard_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s open/erase/write/read/close", argv[0]);
        return 0;
    }
    
    if (0 == htd_memcmp(argv[1], "open", 5)) {
        if (sdcard_handle) {
            htd_dev_sdcard_close(sdcard_handle);
        }
        sdcard_handle = htd_dev_sdcard_open();
        ASSERT(sdcard_handle);
        
    } else if (0 == htd_memcmp(argv[1], "erase", 6)) {
        // 擦除
        htd_dev_sdcard_block_erase(sdcard_handle, block_idx, 10);
        print_warning("erase finish.");
        
    } else if (0 == htd_memcmp(argv[1], "write", 6)) {
        // 写入数据
        dev_sdcard_info_t sdcard_info;
        uint8_t *p_data = (uint8_t*)htd_malloc(rwdata_len);
        ASSERT(p_data);
        for (uint32_t i=0; i<rwdata_len; ++i) {p_data[i] = i & 0x1f;}
        htd_dev_sdcard_info_get(sdcard_handle, &sdcard_info);
        htd_dev_sdcard_block_write(sdcard_handle, block_idx, p_data, rwdata_len/sdcard_info.block_size);
        htd_free(p_data);
        print_warning("write finish. rel_card_addr=0x%08x", sdcard_info.rel_card_addr);
        
    } else if (0 == htd_memcmp(argv[1], "read", 5)) {
        dev_sdcard_info_t sdcard_info;
        uint8_t *p_data = (uint8_t*)htd_malloc(rwdata_len);
        ASSERT(p_data);
        htd_memset(p_data, 0, rwdata_len);
        htd_dev_sdcard_info_get(sdcard_handle, &sdcard_info);
        htd_dev_sdcard_block_read(sdcard_handle, block_idx, p_data, rwdata_len/sdcard_info.block_size);
        
        print_warning("read finish. ");
        print_hex("data", p_data, rwdata_len);
        htd_free(p_data);

    } else if (0 == htd_memcmp(argv[1], "close", 6)) {
        if (sdcard_handle) {
            htd_dev_sdcard_close(sdcard_handle);
            sdcard_handle = NULL;
        }
        
    } else {
        print_error("Usage: %s open/erase/write/read/close", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(htd_sdcard_cmd, sdcard);
#endif /**/

