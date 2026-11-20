#define DEBUG_LVL           4
#define DEBUG_TAG           "baro.mspc110"
#include "htd_barometer_dev.h"

#if 0
#include <stdint.h>
#include <stdio.h>
#include <math.h>

// --- 硬件配置 ---
// MSPC110 的默认 I2C 地址通常是 0x76 或 0x77，请根据实际模块确认
#define MSPC110_I2C_ADDR    0x76 

// --- 寄存器地址定义 ---
#define REG_STATUS          0x00
#define REG_PRESSURE_LSB    0x01
#define REG_PRESSURE_MSB    0x02
#define REG_TEMP_LSB        0x03
#define REG_TEMP_MSB        0x04
#define REG_CONFIG          0x05 // 配置寄存器 (控制采样率)

// --- 命令/配置位 ---
// 配置寄存器位：触发测量
#define CMD_START_PRESS     0x01 // 启动压力转换
#define CMD_START_TEMP      0x02 // 启动温度转换
#define CMD_SOFT_RESET      0x80 // 软复位

// --- 状态寄存器位 ---
#define STATUS_RDY_PRESS    0x01 // 压力数据就绪
#define STATUS_RDY_TEMP     0x02 // 温度数据就绪

// --- 全局变量 ---
// 海平面基准气压 (单位: hPa)
// 标准大气压为 1013.25 hPa，实际应用建议根据当地天气调整
float g_sea_level_pressure = 1013.25f;

// --- 模拟 I2C 驱动 (请替换为实际 MCU 的 HAL 库) ---
void I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data) {
    // TODO: 实现 I2C 写入一个字节
}

uint8_t I2C_Read_Byte(uint8_t addr, uint8_t reg) {
    // TODO: 实现 I2C 读取一个字节
    return 0; 
}

void I2C_Read_Bytes(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len) {
    // TODO: 实现 I2C 连续读取
}

void Delay_ms(uint32_t ms) {
    // TODO: 实现延时
}

/**
 * @brief 复位传感器
 */
void MSPC110_Reset(void) {
    I2C_Write_Byte(MSPC110_I2C_ADDR, REG_CONFIG, CMD_SOFT_RESET);
    Delay_ms(10); // 等待复位完成
}

/**
 * @brief 启动压力和温度测量
 */
void MSPC110_Start_Measurement(void) {
    // 同时启动压力和温度转换 (具体取决于芯片手册，有些需要分步启动)
    // 这里假设配置寄存器写入触发位
    I2C_Write_Byte(MSPC110_I2C_ADDR, REG_CONFIG, CMD_START_PRESS | CMD_START_TEMP);
}

/**
 * @brief 检查数据是否就绪
 * @return 1: 就绪, 0: 未就绪
 */
uint8_t MSPC110_Is_Data_Ready(void) {
    uint8_t status = I2C_Read_Byte(MSPC110_I2C_ADDR, REG_STATUS);
    // 检查压力和温度数据是否都就绪
    if ((status & STATUS_RDY_PRESS) && (status & STATUS_RDY_TEMP)) {
        return 1;
    }
    return 0;
}

/**
 * @brief 读取压力值和温度值
 * @param pressure_hPa 输出：压力 (hPa)
 * @param temperature_C 输出：温度 (°C)
 */
void MSPC110_Read_Data(float *pressure_hPa, float *temperature_C) {
    uint8_t data[2];
    int32_t raw_pressure = 0;
    int32_t raw_temp = 0;
    
    // 1. 读取压力 (MSB, LSB)
    // 注意：MSPC110 数据格式通常是 16位或24位，这里假设读取 16位有效数据
    // 具体位移需参考芯片数据手册
    I2C_Read_Bytes(MSPC110_I2C_ADDR, REG_PRESSURE_MSB, data, 2);
    raw_pressure = ((int32_t)data[0] << 8) | data[1];
    
    // 2. 读取温度 (MSB, LSB)
    I2C_Read_Bytes(MSPC110_I2C_ADDR, REG_TEMP_MSB, data, 2);
    raw_temp = ((int32_t)data[0] << 8) | data[1];

    // 3. 数据转换 (根据 MSPC110 数据手册的公式)
    // 假设压力输出单位是 Pa，需要转换为 hPa (除以 100)
    // 假设温度是开尔文或摄氏度，这里做通用转换
    // *** 注意：以下转换系数仅为示例，请查阅具体芯片手册修正 ***
    
    // 示例转换：假设 LSB = 1 Pa
    float p_pa = (float)raw_pressure; 
    *pressure_hPa = p_pa / 100.0f;
    
    // 示例转换：假设 LSB = 0.01 °C
    *temperature_C = (float)raw_temp / 100.0f;
}

/**
 * @brief 计算海拔高度
 * 使用国际大气模型公式
 * @param pressure_hPa 当前气压
 * @return 高度 (米)
 */
float MSPC110_Calculate_Altitude(float pressure_hPa) {
    // 公式: H = 44330 * (1 - (P / P0)^(1/5.255))
    // P = 当前气压
    // P0 = 海平面标准气压
    
    if (pressure_hPa <= 0) return 0.0f;
    
    float ratio = pressure_hPa / g_sea_level_pressure;
    float altitude = 44330.0f * (1.0f - powf(ratio, 1.0f / 5.255f));
    
    return altitude;
}

// --- 主函数示例 ---
int main(void) {
    float pressure, temperature, altitude;
    
    // 1. 初始化
    // I2C_Init(); 
    MSPC110_Reset();
    
    printf("MSPC110 传感器初始化完成\r\n");

    while (1) {
        // 2. 启动测量
        MSPC110_Start_Measurement();
        
        // 3. 等待数据就绪 (简单轮询，实际可用中断)
        while (!MSPC110_Is_Data_Ready()) {
            Delay_ms(1);
        }
        
        // 4. 读取数据
        MSPC110_Read_Data(&pressure, &temperature);
        
        // 5. 计算高度
        altitude = MSPC110_Calculate_Altitude(pressure);
        
        // 6. 输出结果
        printf("气压: %.2f hPa | 温度: %.2f C | 高度: %.2f m\r\n", 
               pressure, temperature, altitude);
        
        Delay_ms(1000); // 每秒刷新一次
    }
}



#endif
