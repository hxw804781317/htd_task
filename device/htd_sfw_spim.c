// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "sfw.spim"
#include "htd_device.h"
#include "htd_dev_spim.h"


#if (defined(BSP_USING_SFW_SPIM0) || defined(BSP_USING_SFW_SPIM1) || defined(BSP_USING_SFW_SPIM2) || defined(BSP_USING_SFW_SPIM3) || defined(BSP_USING_SFW_SPIM4))
//////////////////////////////////////////////////////////////////////
enum {
#ifdef BSP_USING_SFW_SPIM0
    SPIM0_SFW_INDEX,
#endif
#ifdef BSP_USING_SFW_SPIM1
    SPIM1_SFW_INDEX,
#endif
#ifdef BSP_USING_SFW_SPIM2
    SPIM2_SFW_INDEX,
#endif
#ifdef BSP_USING_SFW_SPIM3
    SPIM3_SFW_INDEX,
#endif
#ifdef BSP_USING_SFW_SPIM4
    SPIM4_SFW_INDEX,
#endif
    SPIM_SFW_COUNT,
};

typedef struct spi_master_s sfw_spim_t;
typedef uint8_t (*spim_tx_rx_t)(const sfw_spim_t*, uint8_t);
struct spi_master_s {
    uint8_t reference;
    uint32_t nop_count;
    spim_tx_rx_t handle;
    spim_handle_t* who;
    htd_mutex_lock_t *lock;
};
static sfw_spim_t m_spim_sfw_object[SPIM_SFW_COUNT];


// @brief 延迟
static void sfw_spim_delay(const sfw_spim_t* p_object)
{
    uint32_t n = p_object->nop_count;
    while (n--) {
        __ASM("nop");
    }
    //htd_delay_us(1);
}

// @brief SPI 模式0(Mode 0 CPOL=0, CPHA=0)
static uint8_t spim0_tx_rx_byte(const sfw_spim_t* p_object, uint8_t data)
{
    // 以MSB方式按位接收一个字节数据,上升沿一位数据被存入移位寄存器
    const spim_user_config_t* p_user_config = p_object->who->p_user_config;
    for (uint32_t i=0; i<8; i++) {
        sfw_spim_delay(p_object);
        if (data & 0x80) {
            htd_pin_write(p_user_config->pin_mosi, 1);
        } else {
            htd_pin_write(p_user_config->pin_mosi, 0);
        }
        sfw_spim_delay(p_object);
        
        htd_pin_write(p_user_config->pin_sck, 1);
        sfw_spim_delay(p_object);
        data <<= 1;
        if (htd_pin_read(p_user_config->pin_miso)) {
            data |= 0x01;
        }
        sfw_spim_delay(p_object);
        htd_pin_write(p_user_config->pin_sck, 0);
        sfw_spim_delay(p_object);
    }
    return data;
}

// @brief SPI 模式1(Mode 1 CPOL=0, CPHA=1)
static uint8_t spim1_tx_rx_byte(const sfw_spim_t* p_object, uint8_t data)
{
    // 以MSB方式按位接收一个字节数据,上升沿一位数据被存入移位寄存器
    const spim_user_config_t* p_user_config = p_object->who->p_user_config;
    for (uint32_t i=0; i<8; i++) {
        sfw_spim_delay(p_object);
        htd_pin_write(p_user_config->pin_sck, 1);
        if (data & 0x80) {
            htd_pin_write(p_user_config->pin_mosi, 1);
        } else {
            htd_pin_write(p_user_config->pin_mosi, 0);
        }
        sfw_spim_delay(p_object);
        
        htd_pin_write(p_user_config->pin_sck, 0);
        sfw_spim_delay(p_object);
        data <<= 1;
        if (htd_pin_read(p_user_config->pin_miso)) {
            data |= 0x01;
        }
    }
    return data;
}

// @brief SPI 模式2(Mode 2 CPOL=1, CPHA=0)
static uint8_t spim2_tx_rx_byte(const sfw_spim_t* p_object, uint8_t data)
{
    // 以MSB方式按位接收一个字节数据,上升沿一位数据被存入移位寄存器
    const spim_user_config_t* p_user_config = p_object->who->p_user_config;
    for(uint32_t i=0; i<8; i++) {
        sfw_spim_delay(p_object);
        if (data & 0x80) {
            htd_pin_write(p_user_config->pin_mosi, 1);
        } else {
            htd_pin_write(p_user_config->pin_mosi, 0);
        }
        sfw_spim_delay(p_object);
        
        htd_pin_write(p_user_config->pin_sck, 0);
        sfw_spim_delay(p_object);
        data <<= 1;
        if (htd_pin_read(p_user_config->pin_miso)) {
            data |= 0x01;
        }
        htd_pin_write(p_user_config->pin_sck, 1);
        sfw_spim_delay(p_object);
    }
    return data;
}

// @brief SPI 模式3(Mode 3 CPOL=1, CPHA=1)
static uint8_t spim3_tx_rx_byte(const sfw_spim_t* p_object, uint8_t data)
{
    // 以MSB方式按位接收一个字节数据,上升沿一位数据被存入移位寄存器
    const spim_user_config_t* p_user_config = p_object->who->p_user_config;
    for (uint32_t i=0; i<8; i++) {
        sfw_spim_delay(p_object);
        htd_pin_write(p_user_config->pin_sck, 0);
        sfw_spim_delay(p_object);
        if (data & 0x80) {
            htd_pin_write(p_user_config->pin_mosi, 1);
        } else {
            htd_pin_write(p_user_config->pin_mosi, 0);
        }
        sfw_spim_delay(p_object);
        
        htd_pin_write(p_user_config->pin_sck, 1);
        sfw_spim_delay(p_object);
        data <<= 1;
        if (htd_pin_read(p_user_config->pin_miso)) {
            data |= 0x01;
        }
    }
    return data;
}

// @brief 读写一个字节
uint8_t spim_tx_rx_byte(const sfw_spim_t* p_object, uint8_t data)
{
    const spim_user_config_t* p_user_config = p_object->who->p_user_config;
    for (uint32_t i=0; i<8; i++) {
        sfw_spim_delay(p_object);
        if (0x80 & data) {
            htd_pin_write(p_user_config->pin_mosi, 1);
        } else { 
            htd_pin_write(p_user_config->pin_mosi, 0);
        }
        sfw_spim_delay(p_object);
        htd_pin_write(p_user_config->pin_sck, 1);
        sfw_spim_delay(p_object);
        data <<= 1;
        if (htd_pin_read(p_user_config->pin_miso)) {
            data |= 1; 
        }
        sfw_spim_delay(p_object);
        htd_pin_write(p_user_config->pin_sck, 0);
        sfw_spim_delay(p_object);
    }
    return data;
}


//////////////////////////////////////////////////////////////////
// @brief 控制CS线
static void sfw_spim_cs_enable_set(spim_handle_t* p_handle, bool active)
{
    const spim_user_config_t* p_user_config = p_handle->p_user_config;
    if (p_user_config->pin_cs != PIN_NUM_INVALID) {
        if (active) {
            htd_pin_write(p_user_config->pin_cs, p_user_config->cs_active_level ? 1 : 0);
            print_debug("===>cs enable. pin_level=%d", p_user_config->cs_active_level ? 1 : 0);
        } else {
            htd_pin_write(p_user_config->pin_cs, p_user_config->cs_active_level ? 0 : 1);
            print_debug("===>cs disable. pin_level=%d", p_user_config->cs_active_level ? 0 : 1);
        }
    }
}

// @brief 默认配置
static void sfw_spim_param_init(spim_handle_t* p_handle, sfw_spim_t* p_object)
{
    const spim_user_config_t* p_user_config = p_handle->p_user_config;
    p_object->nop_count = htd_rcc_freq_hz_get() / p_user_config->freq_hz;
    p_object->nop_count /= 3;
    if (p_object->nop_count > 20) {
        p_object->nop_count -= 20;
    } else {
        p_object->nop_count = 0;
    }
    
    if (p_user_config->mode == SPIM_MODE_1) {
        p_object->handle = spim1_tx_rx_byte;
    } else if (p_user_config->mode == SPIM_MODE_2) {
        p_object->handle = spim2_tx_rx_byte;
    } else if (p_user_config->mode == SPIM_MODE_3) {
        p_object->handle = spim3_tx_rx_byte;
    } else {
        p_object->handle = spim0_tx_rx_byte;
    }
    p_object->who = p_handle;
    print_debug("spi config. mode=%d", p_user_config->mode);
}

// @brief 初始化flash 接口
static int32_t htd_sfw_spim_open(spim_handle_t* p_handle)
{
    ASSERT(p_handle);

    sfw_spim_t* p_object = (sfw_spim_t*)p_handle->p_dev->p_object;
    const spim_user_config_t* p_user_config = p_handle->p_user_config;
    ASSERT(p_user_config->is_bit_order_msb > 0);
    ASSERT(p_user_config->data_width == 8);
    ASSERT(p_user_config->is_1line == 0);

    if (p_object->lock == NULL) {
        p_object->lock = htd_mutex_lock_create(p_handle->p_dev->p_name);
        ASSERT(p_object->lock);
    }

    // 控制CS线
    htd_mutex_lock(p_object->lock);
    if (p_user_config->pin_cs != PIN_NUM_INVALID) {
        htd_pin_output_pp_set(p_user_config->pin_cs);
        htd_pin_write(p_user_config->pin_cs, p_user_config->cs_active_level ? 0 : 1);
    }
    
    if (p_object->who != NULL) {
        if (p_object->who == p_handle) {
            print_error("sfw spim(%s) is opened.", p_handle->p_dev->p_name);
            htd_mutex_unlock(p_object->lock);
            return -1;
        }
        p_object->reference++;
        ASSERT(p_object->who->p_user_config->pin_miso == p_user_config->pin_miso);
        ASSERT(p_object->who->p_user_config->pin_mosi == p_user_config->pin_mosi);
        ASSERT(p_object->who->p_user_config->pin_sck == p_user_config->pin_sck);
        htd_mutex_unlock(p_object->lock);
        return 0;
    } 
    
    print_debug("open sfw spi. sck=%d, mosi=%d, miso=%d", p_user_config->pin_sck, p_user_config->pin_mosi, p_user_config->pin_miso);
    // 引脚配置
    htd_pin_output_pp_set(p_user_config->pin_sck);
    htd_pin_output_pp_set(p_user_config->pin_mosi);
    htd_pin_input_set(p_user_config->pin_miso, HTD_PIN_PULLUP);
    htd_pin_write(p_user_config->pin_sck, 0);
    htd_pin_write(p_user_config->pin_mosi, 0);
    p_object->reference++;
    p_object->who = p_handle;
    sfw_spim_param_init(p_handle, p_object);
    htd_mutex_unlock(p_object->lock);
    return 0;
}

// @brief 去初始化
static int32_t htd_sfw_spim_close(spim_handle_t* p_handle)
{
    sfw_spim_t* p_object = (sfw_spim_t*)p_handle->p_dev->p_object;

    htd_mutex_lock(p_object->lock);
    const spim_user_config_t* p_user_config = p_object->who->p_user_config;
    if (p_object->reference == 1) {
        htd_pin_write(p_user_config->pin_sck, 0);
        htd_pin_write(p_user_config->pin_mosi, 0);
        // 拉低MISO引脚
        htd_pin_output_pp_set(p_user_config->pin_miso);
        htd_pin_write(p_user_config->pin_miso, 0);
        // CS 引脚关闭
        sfw_spim_cs_enable_set(p_handle, false);
        p_object->who = NULL;
        p_object->reference = 0;
        print_debug("close spi.");
    } else {
        p_object->reference--;
    }
    htd_mutex_unlock(p_object->lock);
    return 0;
}

// @brief 发送数据，必须为阻塞模式
static int32_t htd_sfw_spim_write(sfw_spim_t* p_object, const uint8_t* p_data, uint32_t data_len)
{
    print_hex("spim write", p_data, data_len);
    const uint8_t* p_start = p_data;
    if (p_data) {
        while (data_len--) {
            p_object->handle(p_object, *p_data++);
        }
    }
    return p_data - p_start;
}

// @brief 接收数据，必须为阻塞模式
static int32_t htd_sfw_spim_read(sfw_spim_t* p_object, uint8_t* p_data, uint32_t data_size)
{
    const uint8_t* p_start = p_data;
    if (p_data){
        while (data_size--) {
            *p_data++ = p_object->handle(p_object, 0xff);
        }
    }
    //print_hex("spim read", p_start, p_data-p_start);
    return p_data - p_start;
}

// @brief 收发控制, 返回发送的个数
static int32_t htd_sfw_spim_xfer(spim_handle_t* p_handle, spim_msg_t* p_msg, uint32_t msg_count)
{
    uint32_t count = msg_count;
    sfw_spim_t* p_object = (sfw_spim_t*)p_handle->p_dev->p_object;

    htd_mutex_lock(p_object->lock);
    if (p_object->who != p_handle) {
        sfw_spim_param_init(p_handle, p_object);
    }
    
    sfw_spim_cs_enable_set(p_handle, true);
    while (msg_count--) {
        if (p_msg->flags & SPIM_MSG_FLAG_TX) {
            htd_sfw_spim_write(p_object, p_msg->buff, p_msg->len);
        } else {
            htd_sfw_spim_read(p_object, p_msg->buff, p_msg->len);
        }
        if (p_handle->p_user_config->xfer_finish) {
            p_handle->p_user_config->xfer_finish(p_handle, p_msg->buff, p_msg->len);
        }
        p_msg++;
    }
    sfw_spim_cs_enable_set(p_handle, false);
    htd_mutex_unlock(p_object->lock);
    return count;
}

// @brief 控制句柄
int32_t htd_sfw_spim_ctrl(void* handle, uint32_t cmd, void* p_param, uint32_t param_len)
{
    spim_handle_t* p_handle = (spim_handle_t*)handle;
    if (cmd == HTD_SPIM_CMD_OPEN) {
        return htd_sfw_spim_open(p_handle);
    } else if (cmd == HTD_SPIM_CMD_CLOSE) {
        return htd_sfw_spim_close(p_handle);
    } else if (cmd == HTD_SPIM_CMD_XFER) {
        return htd_sfw_spim_xfer(p_handle, (spim_msg_t*)p_param, param_len);
    } else {
        return (-ERROR_CODE_NOT_FOUND);
    }
}

// @brief 初始化
int32_t htd_sfw_spim_init(void)
{
    for (uint32_t i=0; i<ARRAY_COUNT(m_spim_sfw_object); ++i) {
        htd_memset(&m_spim_sfw_object[i], 0, sizeof(sfw_spim_t));
    }
    return 0;
}
DEVICE_INIT_FUNC_REGISTER(htd_sfw_spim_init);


#if defined(BSP_USING_SFW_SPIM0)
    HTD_DEV_REGISTER(sfw_spim0, HTD_SPIM_DEV_IDENTIFY, htd_sfw_spim_ctrl, &m_spim_sfw_object[SPIM0_SFW_INDEX]);
#endif
#if defined(BSP_USING_SFW_SPIM1)
    HTD_DEV_REGISTER(sfw_spim1, HTD_SPIM_DEV_IDENTIFY, htd_sfw_spim_ctrl, &m_spim_sfw_object[SPIM1_SFW_INDEX]);
#endif
#if defined(BSP_USING_SFW_SPIM2)
    HTD_DEV_REGISTER(sfw_spim2, HTD_SPIM_DEV_IDENTIFY, htd_sfw_spim_ctrl, &m_spim_sfw_object[SPIM2_SFW_INDEX]);
#endif
#if defined(BSP_USING_SFW_SPIM3)
    HTD_DEV_REGISTER(sfw_spim3, HTD_SPIM_DEV_IDENTIFY, htd_sfw_spim_ctrl, &m_spim_sfw_object[SPIM3_SFW_INDEX]);
#endif
#if defined(BSP_USING_SFW_SPIM4)
    HTD_DEV_REGISTER(sfw_spim4, HTD_SPIM_DEV_IDENTIFY, htd_sfw_spim_ctrl, &m_spim_sfw_object[SPIM4_SFW_INDEX]);
#endif
#endif /*(defined(BSP_USING_SFW_SPIM0) || defined(BSP_USING_SFW_SPIM1) || defined(BSP_USING_SFW_SPIM2) || defined(BSP_USING_SFW_SPIM3) || defined(BSP_USING_SFW_SPIM4))*/

