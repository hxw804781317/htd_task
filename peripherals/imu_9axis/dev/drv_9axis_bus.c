#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "9axis.bus"
#include "htd_common.h"
#include "drv_9axis_bus.h"
#include "htd_device.h"


#if defined(HTD_9AXIS_BUS_I2C)
//////////////////////////////////////////////////////////////////////
static i2cm_handle_t* m_9axis_bus = NULL;

// @brief 总线初始化
int32_t drv_9axis_bus_init(void)
{
    static const i2cm_user_config_t i2c_config = {
        .pin_sck = HTD_9AXIS_I2C_PIN_SCK,
        .pin_sda = HTD_9AXIS_I2C_PIN_SDA,
        .MSB = 1,
        .freq_hz = 400*1000,
    };
    if (m_9axis_bus == NULL) {
        m_9axis_bus = htd_i2cm_open(HTD_9AXIS_BUS_NAME, &i2c_config);
        htd_delay_us(10);
    }
    return 0;
}

// @brief 总线去初始化
int32_t drv_9axis_bus_deinit(void)
{
    if (m_9axis_bus) {
        htd_i2cm_close(m_9axis_bus);
        m_9axis_bus = NULL;
    }
    return 0;
}

// @brief 总线读数据
int32_t drv_9axis_bus_data_read(uint8_t reg, uint8_t* p_data, uint16_t data_size)
{
    return htd_i2cm_reg8_read(m_9axis_bus, MPU6050_ADDRESS, reg, p_data, data_size);
}

// @brief 总线读数据
static int32_t drv_9axis_bus_data_write(uint8_t reg, const uint8_t* p_data, uint16_t data_size)
{
    return htd_i2cm_reg8_write(m_9axis_bus, MPU6050_ADDRESS, reg, p_data, data_size);
}

// @brief 写寄存器地址
int32_t drv_9axis_register_write(uint8_t reg, uint8_t val)
{
    return drv_9axis_bus_data_write(reg, &val, 1);
}

// @brief 读寄存器
int32_t drv_9axis_register_read(uint8_t reg, uint8_t* p_data)
{
    return drv_9axis_bus_data_read(reg, p_data, 1);
}


#else
static spim_handle_t* m_9axis_bus = NULL;

// @brief 总线初始化
int32_t drv_9axis_bus_init(void)
{
    static const spim_user_config_t spi_config = {
        .pin_cs = HTD_9AXIS_SPI_PIN_CS,
        .pin_sck = HTD_9AXIS_SPI_PIN_SCK,
        .pin_mosi = HTD_9AXIS_SPI_PIN_MOSI,
        .pin_miso = HTD_9AXIS_SPI_PIN_MISO,
        .cs_active_level = 0,
        .is_1line = 0,
        .is_bit_order_msb = 1,
        .data_width = 8,
        .mode = SPIM_MODE_0,
        .freq_hz = 8*1000*1000,
        .xfer_finish = NULL,
    };
    if (m_9axis_bus == NULL) {
        m_9axis_bus = htd_spim_open(HTD_9AXIS_BUS_NAME, &spi_config);
        htd_delay_us(10);
    }
    return 0;
}

// @brief 总线去初始化
int32_t drv_9axis_bus_deinit(void)
{
    if (m_9axis_bus) {
        htd_spim_close(m_9axis_bus);
        m_9axis_bus = NULL;
    }
    return 0;
}

// @brief 总线读数据
int32_t drv_9axis_bus_data_read(uint8_t reg, uint8_t* p_data, uint16_t data_size)
{
    reg |= 0x80;
    return htd_spim_transfer(m_9axis_bus, &reg, sizeof(uint8_t), p_data, data_size);
}

// @brief 写寄存器地址
int32_t drv_9axis_register_write(uint8_t reg, uint8_t val)
{
    uint8_t buff[2];
    buff[0] = reg;
    buff[1] = val;
    return htd_spim_transfer(m_9axis_bus, buff, sizeof(buff), NULL, 0);
}

// @brief 读寄存器
uint8_t drv_9axis_register_read(uint8_t reg)
{
    uint8_t data;
    reg |= 0x80;
    htd_spim_transfer(m_9axis_bus, &reg, sizeof(uint8_t), &data, sizeof(uint8_t));
    return data;
}
#endif


// @brief 上电控制
void drv_9axis_power_ctrl(bool enable)
{
#if ((defined HTD_9AXIS_POWER_PIN) && (HTD_9AXIS_POWER_PIN > 0))
    uint8_t level = !!enable;
  #if (!defined(HTD_9AXIS_POWER_PIN_ACTIVE_HIGH))
    level = !level;
  #endif
    htd_pin_output_pp_set(HTD_9AXIS_POWER_PIN);
    htd_pin_write(HTD_9AXIS_POWER_PIN, level);
    print_warning("mpu6050 power ctrl. enable=%d", enable);
    if (enable) {
        htd_sleep_ms(20);
    } 
#endif 
}


