
#if 0
#include <stdint.h>
#include <stdio.h>
#include <math.h>

// --- 1. 硬件配置与寄存器定义 ---

// LPS27HHW 默认 I2C 地址 (当 SA0 接地时为 0x5C, 接 VCC 时为 0x5D)
#define LPS27HHW_I2C_ADDR   0x5C

// 寄存器地址
#define REG_WHO_AM_I        0x0F
#define REG_CTRL            0x10
#define REG_CTRL2           0x11
#define REG_IF_CTRL         0x19
#define REG_STATUS          0x25
#define REG_PRESS_OUT_XL    0x20 // 压力低字节
#define REG_PRESS_OUT_L     0x21 // 压力中字节
#define REG_PRESS_OUT_H     0x22 // 压力高字节
#define REG_TEMP_OUT_L      0x2A
#define REG_TEMP_OUT_H      0x2B

// 命令位
#define CMD_BDU             0x02 // 块数据更新
#define CMD_ONE_SHOT        0x08 // 单次触发模式
#define CMD_IF_ADD_INC      0x04 // I2C 地址自动递增

// 预期 WHO_AM_I 值
#define LPS27HHW_ID         0xB3

// --- 2. 模拟硬件抽象层 (HAL) ---
// 在实际项目中，请替换为你的 STM32 HAL 或 ESP-IDF 等驱动

// 写入一个字节
void I2C_Write_Byte(uint8_t dev_addr, uint8_t reg, uint8_t data) {
    // TODO: 实现 I2C 写入
    // HAL_I2C_Mem_Write(&hi2c1, dev_addr, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

// 读取多个字节
void I2C_Read_Bytes(uint8_t dev_addr, uint8_t reg, uint8_t *buffer, uint8_t len) {
    // TODO: 实现 I2C 读取
    // HAL_I2C_Mem_Read(&hi2c1, dev_addr, reg, I2C_MEMADD_SIZE_8BIT, buffer, len, 100);
}

void Delay_ms(uint32_t ms) {
    // TODO: 实现延时
}

// --- 3. 驱动逻辑 ---

/**
 * @brief 初始化 LPS27HHW
 * @return 1: 成功, 0: 失败
 */
uint8_t LPS27HHW_Init(void) {
    uint8_t who_am_i = 0;
    
    // 1. 读取 WHO_AM_I 确认通信
    I2C_Read_Bytes(LPS27HHW_I2C_ADDR, REG_WHO_AM_I, &who_am_i, 1);
    if (who_am_i != LPS27HHW_ID) {
        return 0; // 传感器未找到或型号错误
    }

    // 2. 配置控制寄存器
    // 设置 BDU (Block Data Update) 防止读取时数据变化
    // 设置 ONE_SHOT (单次模式) 以节省功耗
    I2C_Write_Byte(LPS27HHW_I2C_ADDR, REG_CTRL, CMD_BDU | CMD_ONE_SHOT);
    
    // 3. 配置接口控制寄存器
    // 开启 I2C 地址自动递增，以便连续读取多个寄存器
    I2C_Write_Byte(LPS27HHW_I2C_ADDR, REG_IF_CTRL, CMD_IF_ADD_INC);

    return 1;
}

/**
 * @brief 触发一次测量并读取数据
 * @param pressure_hPa 输出：气压值 (hPa)
 * @param temperature_C 输出：温度值 (°C)
 */
void LPS27HHW_Read_Data(float *pressure_hPa, float *temperature_C) {
    uint8_t buffer[6];
    uint32_t raw_pressure = 0;
    int16_t raw_temp = 0;
    
    // 1. 触发单次测量
    // 向 CTRL 寄存器写入配置以启动转换
    I2C_Write_Byte(LPS27HHW_I2C_ADDR, REG_CTRL, CMD_BDU | CMD_ONE_SHOT);
    
    // 2. 等待数据就绪 (轮询状态寄存器)
    // 简单延时替代轮询，实际应用中建议读取 STATUS 寄存器检查 P_DA 位
    Delay_ms(20); // LPS27HHW 典型转换时间约为 10-15ms (取决于平均次数)

    // 3. 连续读取压力和温度数据
    // 从 REG_PRESS_OUT_XL 开始读取 6 个字节 (3字节压力 + 2字节温度 + 1字节空)
    I2C_Read_Bytes(LPS27HHW_I2C_ADDR, REG_PRESS_OUT_XL, buffer, 6);

    // 4. 解析压力数据 (Little Endian)
    // LPS27HHW 输出单位直接为 hPa * 100 (即 1 hPa = 100 LSB)
    // 实际上 LPS27HHW 输出的是 mbar (hPa)，分辨率为 1/100 hPa
    raw_pressure = ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[1] << 8) | (uint32_t)buffer[0];
    
    // 转换为 hPa (浮点数)
    *pressure_hPa = (float)raw_pressure / 100.0f;

    // 5. 解析温度数据 (Little Endian, 有符号数)
    raw_temp = (int16_t)(((uint16_t)buffer[5] << 8) | (uint16_t)buffer[4]);
    
    // 转换为摄氏度 (LPS27HHW 分辨率为 100 LSB/°C)
    *temperature_C = (float)raw_temp / 100.0f;
}

/**
 * @brief 计算海拔高度
 * @param pressure_hPa 当前气压 (hPa)
 * @param sea_level_hPa 海平面基准气压 (hPa)
 * @return 高度 (米)
 */
float LPS27HHW_Calculate_Altitude(float pressure_hPa, float sea_level_hPa) {
    // 国际大气模型公式
    // H = 44330 * (1 - (P / P0)^(1/5.255))
    
    if (pressure_hPa <= 0) return 0.0f;

    float ratio = pressure_hPa / sea_level_hPa;
    float altitude = 44330.0f * (1.0f - powf(ratio, 1.0f / 5.255f));
    
    return altitude;
}

// --- 4. 主程序示例 ---

int main(void) {
    float pressure, temperature, altitude;
    
    // 假设海平面标准气压 (可根据当地天气调整)
    float local_qnh = 1013.25f; 

    // 1. 初始化传感器
    if (LPS27HHW_Init()) {
        printf("LPS27HHW 初始化成功\r\n");
    } else {
        printf("LPS27HHW 初始化失败!\r\n");
        while(1); // 卡死等待
    }

    while (1) {
        // 2. 读取数据
        LPS27HHW_Read_Data(&pressure, &temperature);

        // 3. 计算高度
        altitude = LPS27HHW_Calculate_Altitude(pressure, local_qnh);

        // 4. 打印结果
        printf("气压: %.2f hPa | 温度: %.2f C | 高度: %.2f m\r\n", 
               pressure, temperature, altitude);

        Delay_ms(1000);
    }
}
#endif

