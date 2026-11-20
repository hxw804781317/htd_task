#ifndef __DRV_ICM20948_H__
#define __DRV_ICM20948_H__

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////接口配置/////////////////////////////////////////
#define ICM20948_PIN_AD0_LEVEL                  (0)         // AD0 引脚高低
#define ICM20948_I2C_ADDR                       (0x68|ICM20948_PIN_AD0_LEVEL)
#define AK09916_MAGN_ADDR                       (0x0C)

#define ICM20948_0x7f_REG_BANK_SEL              (0x7f)
#define USER_BANK_0                             (0x00)
#define USER_BANK_1                             (0x10)
#define USER_BANK_2                             (0x20)
#define USER_BANK_3                             (0x30)



/////////////////////////////USER BANK 0 REGISTER/////////////////////////////////////////
// 设备id
#define ICM20948B0_0x00_WHO_AM_I                (0x00)          // 默认值0xEA
// 用户控制寄存器
#define ICM20948B0_0x03_USER_CTRL               (0x03)
    #define USER_CTRL_BIT_DMP_EN                (0x80)          // 使能 DMP
    #define USER_CTRL_BIT_FIFO_EN               (0x40)          // 使能 fifo
    #define USER_CTRL_BIT_I2C_MST_EN            (0x20)          // 使能 i2c 主模式
    #define USER_CTRL_BIT_I2C_IF_DIS            (0x10)          // 关闭 i2c 从模式，只能使用 spi 单一模式
    #define USER_CTRL_BIT_DMP_RST               (0x08)          // 复位 DMP
    #define USER_CTRL_BIT_SRAM_RST              (0x04)          // 复位 SRAM
    #define USER_CTRL_BIT_I2C_MST_RST           (0x02)          // 复位 i2c 主模式，
// 配置设备进入LP模式
#define ICM20948B0_0x05_LP_CONFIG               (0x05)
    #define LP_CONFIG_BIT_I2C_MST_CYCLE         (0x40)          // 使能I2C master循环模式
    #define LP_CONFIG_BIT_ACCEL_CYCLE           (0x20)          // 使能加速度循环模式
    #define LP_CONFIG_BIT_GYRO_CYCLE            (0x10)          // 使能陀螺仪循环模式
// 电源管理控制寄存器1
#define ICM20948B0_0x06_PWR_MGMT1               (0x06)
    #define PWR_MGMT1_BIT_RESET                 (0x80)          // 复位整个芯片，所有数值恢复默认值
    #define PWR_MGMT1_BIT_SLEEP                 (0x40)          // 休眠标记位，设置后将进入休眠状态
    #define PWR_MGMT1_BIT_LP_EN                 (0x20)          // 低电标记位，当设备配置为LP模式后，可以使能该标记位降低整体功耗
    #define PWR_MGMT1_BIT_TEMP_DIS              (0x08)          // 禁止温度检测
    #define PWR_MGMT1_MSK_CLOCK_SRC             (0x07)          // 时钟源；0/6->内部20M时钟源
                                                                // 1~5：
                                                                // 7: 停止时钟并保持定时发生器复位
// 电源管理控制寄存器2
#define ICM20948B0_0x07_PWR_MGMT2               (0x07)
    #define PWR_MGMT2_MSK_ACCEL_DISABLE         (0x38)          // 关闭加速度
    #define PWR_MGMT2_MSK_GYRO_DISABLE          (0x07)          // 关闭陀螺仪
// 中断引脚配置寄存器
#define ICM20948B0_0x0f_INT_PIN_CFG             (0x0f)
    #define INT_CFG_BIT_ACTL                    (0x80)          // 配置中断电平为低电平，默认为高电平
    #define INT_CFG_BIT_OPEN                    (0x40)          // 配置中断为开漏模式，默认为推挽
    #define INT_CFG_BIT_LATCH_INT_EN            (0x20)          // 配置中断电平持续到中断状态清除，默认中断电平持续50us
    #define INT_CFG_BIT_INT_ANYRD_2CLEAR        (0x10)          // 配置为读取数据后中断状态清除，默认为读取中断状态位后再清除
    #define INT_CFG_BIT_ACTL_FSYNC              (0x08)          // 配置 FSY 引脚中断电源为低电平，默认为高电平
    #define INT_CFG_BIT_FSYNC_INT_MODE_EN       (0x04)          // 配置 FSY 中断使能，默认关闭
    #define INT_CFG_BIT_BYPASS_EN               (0x02)          // 配置ES_CL/ES_DA 进入旁路模式，一般使用SPI模式时要使能该位
// 使能中断
#define ICM20948B0_0x10_INT_ENABLE              (0x10)
    #define INT_ENABLE_BIT_REG_WOF_EN           (0x80)          // 使能 FSYNC 中断
    #define INT_ENABLE_BIT_WOM_INT_EN           (0x08)          // 使能运动时产生中断
    #define INT_ENABLE_BIT_PLL_RDY_EN           (0x04)          // 
    #define INT_ENABLE_BIT_DMP_INT1_EN          (0x02)          // 使能DMP中断
    #define INT_ENABLE_BIT_I2C_MST_INT_EN       (0x01)          // 使能I2C主模式中断
// 使能中断
#define ICM20948B0_0x11_INT1_ENABLE             (0x11)
    #define INT1_ENABLE_BIT_RAW_DATA_0_RDY_EN   (0x01)          // 使能原始数据准备就绪产生中断
// 使能中断
#define ICM20948B0_0x12_INT1_ENABLE             (0x12)
    #define INT2_ENABLE_MSK_FIFO_OVERFLOW_EN    (0x0f)          // 使能 fifo 溢出时产生中断
// 使能中断
#define ICM20948B0_0x13_INT1_ENABLE             (0x13)
    #define INT2_ENABLE_MSK_FIFO_WM_EN          (0x0f)
// I2C 主模式状态
#define ICM20948B0_0x17_I2C_MST_STATUS          (0x17)
    #define I2C_MST_STATUS_BIT_PASS_THROUGH     (0x80)
    #define I2C_MST_STATUS_BIT_I2C_SLV4_DONE    (0x40)          // 当 i2c slv4 传输完成是产生中断
    #define I2C_MST_STATUS_BIT_I2C_LOST_ARB     (0x20)
    #define I2C_MST_STATUS_BIT_I2C_SLV4_NACK    (0x10)          // 当没有收到i2c slv4 的ack时, 且 INT_ENABLE_BIT_I2C_MST_INT_EN 使能时，会触发中断
    #define I2C_MST_STATUS_BIT_I2C_SLV3_NACK    (0x08)          // 当没有收到i2c slv3 的ack时, 且 INT_ENABLE_BIT_I2C_MST_INT_EN 使能时，会触发中断
    #define I2C_MST_STATUS_BIT_I2C_SLV2_NACK    (0x04)          // 当没有收到i2c slv2 的ack时, 且 INT_ENABLE_BIT_I2C_MST_INT_EN 使能时，会触发中断
    #define I2C_MST_STATUS_BIT_I2C_SLV1_NACK    (0x02)          // 当没有收到i2c slv1 的ack时, 且 INT_ENABLE_BIT_I2C_MST_INT_EN 使能时，会触发中断
    #define I2C_MST_STATUS_BIT_I2C_SLV0_NACK    (0x01)          // 当没有收到i2c slv0 的ack时, 且 INT_ENABLE_BIT_I2C_MST_INT_EN 使能时，会触发中断
// 中断状态
#define ICM20948B0_0x19_INT_STATUS              (0x19)
    #define INT_STATUS_BIT_WOM_INT              (0x08)
    #define INT_STATUS_BIT_PLL_RDY_INT          (0x04)
    #define INT_STATUS_BIT_DMP_INT1             (0x02)
    #define INT_STATUS_BIT_I2C_MST_INT          (0x01)
// 中断状态
#define ICM20948B0_0x1a_INT_STATUS1             (0x1a)
    #define INT_STATUS1_BIT_RAW_DATA_0_RDY_INT  (0x01)
// 中断状态
#define ICM20948B0_0x1b_INT_STATUS2             (0x1b)
    #define INT_STATUS2_MSK_FIFO_OVERFLOW_INT   (0x0f)
// 中断状态
#define ICM20948B0_0x1c_INT_STATUS3             (0x1c)
    #define INT_STATUS3_MSK_FIFO_WM_INT         (0x0f)
// delay time between FSYNC event and the 1st gyro ODR event
#define ICM20948B0_0x28_DELAY_TIME_H            (0x28)          // 延迟高字节  
#define ICM20948B0_0x29_DELAY_TIME_L            (0x29)          // 延迟低字节
// 加速度数据地址
#define ICM20948B0_0x2d_ACCEL_XOUT_H            (0x2d)
#define ICM20948B0_0x2e_ACCEL_XOUT_L            (0x2e)
#define ICM20948B0_0x2f_ACCEL_YOUT_H            (0x2f)
#define ICM20948B0_0x30_ACCEL_YOUT_L            (0x30)
#define ICM20948B0_0x31_ACCEL_ZOUT_H            (0x31)
#define ICM20948B0_0x32_ACCEL_ZOUT_L            (0x32)
// 陀螺仪数据地址
#define ICM20948B0_0x33_GYRO_XOUT_H             (0x33)
#define ICM20948B0_0x34_GYRO_XOUT_L             (0x34)
#define ICM20948B0_0x35_GYRO_YOUT_H             (0x35)
#define ICM20948B0_0x36_GYRO_YOUT_L             (0x36)
#define ICM20948B0_0x37_GYRO_ZOUT_H             (0x37)
#define ICM20948B0_0x38_GYRO_ZOUT_L             (0x38)
// 温度数据地址
#define ICM20948B0_0x39_TEMPERATURE_H           (0x39)
#define ICM20948B0_0x3a_TEMPERATURE_L           (0x3a)
// 扩展传感器数据(地磁数据)地址0x3b~0x52
#define ICM20948B0_0x3b_EXTEN_DATA              (0x3b)
// 使能 fifo
#define ICM20948B0_0x66_FIFO_EN_1               (0x66)
    #define FIFO_EN_1_BIT_SLV3_FIFO_EN          (0x08)
    #define FIFO_EN_1_BIT_SLV2_FIFO_EN          (0x04)
    #define FIFO_EN_1_BIT_SLV1_FIFO_EN          (0x02)
    #define FIFO_EN_1_BIT_SLV0_FIFO_EN          (0x01)
#define ICM20948B0_0x67_FIFO_EN_2               (0x67)
    #define FIFO_EN_2_BIT_ACCEL_FIFO_EN         (0x10)
    #define FIFO_EN_2_BIT_GYRO_Z_FIFO_EN        (0x08)
    #define FIFO_EN_2_BIT_GYRO_Y_FIFO_EN        (0x04)
    #define FIFO_EN_2_BIT_GYRO_X_FIFO_EN        (0x02)
    #define FIFO_EN_2_BIT_TEMP_FIFO_EN          (0x01)
#define ICM20948B0_0x68_FIFO_RST                (0x68)
// 设置 fifo 模式
#define ICM20948B0_0x69_FIFO_MODE               (0x69)
    
#define ICM20948B0_0x70_FIFO_COUNTH             (0x70)          // fifo 高字节, bit0~4
#define ICM20948B0_0x71_FIFO_COUNTL             (0x71)          // fifo 地字节, bit0~7
#define ICM20948B0_0x72_FIFO_R_W                (0x72)
#define ICM20948B0_0x74_DATA_RDY_STATUS         (0x74)
#define ICM20948B0_0x76_FIFO_CFG                (0x76)



/////////////////////////////USER BANK 1 REGISTER/////////////////////////////////////////
// 出厂自测结果
#define ICM20948B1_0x02_SELF_TEST_X_GYRO        (0x02)          
#define ICM20948B1_0x03_SELF_TEST_Y_GYRO        (0x03)
#define ICM20948B1_0x04_SELF_TEST_Z_GYRO        (0x04)
#define ICM20948B1_0x0E_SELF_TEST_X_ACCEL       (0x0E)
#define ICM20948B1_0x0F_SELF_TEST_Y_ACCEL       (0x0F)
#define ICM20948B1_0x10_SELF_TEST_Z_ACCEL       (0x10)
// 加速度校准值，以+-16g为基准，单位 0.98mg
#define ICM20948B1_0x14_XA_OFFS_H               (0x14)          // 高字节
#define ICM20948B1_0x15_XA_OFFS_L               (0x15)          // 低字节，有效位bit1~bit7
#define ICM20948B1_0x17_YA_OFFS_H               (0x17)
#define ICM20948B1_0x18_YA_OFFS_L               (0x18)
#define ICM20948B1_0x1a_ZA_OFFS_H               (0x1a)
#define ICM20948B1_0x1b_ZA_OFFS_L               (0x1b)
#define ICM20948B1_0x28_TIMEBASE_CORRECTION_PLL (0x28)




/////////////////////////////USER BANK 2 REGISTER/////////////////////////////////////////
// 采样率，只有当GYRO_CFG1_MSK_DLPFCFG配置为1~6，且GYRO_CFG1_BIT_FCHOICE使能时才起作用
#define ICM20948B2_0x00_GYRO_SAMPLE_DIV         (0x00)          // 采样率 = 1125Hz / (1 + div)
// 陀螺仪配置寄存器1
#define ICM20948B2_0x01_GYRO_CONFIG_1           (0x01)
    #define GYRO_CFG1_MSK_DLPFCFG               (0x38)          // 陀螺仪低通滤波配置
    #define GYRO_DLPFCFG_0                      (0x00)
    #define GYRO_DLPFCFG_1                      (0x08)
    #define GYRO_DLPFCFG_2                      (0x10)
    #define GYRO_DLPFCFG_3                      (0x18)
    #define GYRO_DLPFCFG_4                      (0x20)
    #define GYRO_DLPFCFG_5                      (0x28)
    #define GYRO_DLPFCFG_6                      (0x30)
    #define GYRO_DLPFCFG_7                      (0x38)
    #define GYRO_CFG1_MSK_FS_SEL                (0x06)          // 配置陀螺仪量程
    #define GYRO_CFG1_FS_250                    (0x00)          // +-250 dps
    #define GYRO_CFG1_FS_500                    (0x02)          // +-500 dps
    #define GYRO_CFG1_FS_1000                   (0x04)          // +-1000 dps
    #define GYRO_CFG1_FS_2000                   (0x06)          // +-2000 dps
    #define GYRO_CFG1_BIT_FCHOICE               (0x01)
// 陀螺仪配置寄存器2
#define ICM20948B2_0x02_GYRO_CONFIG_2           (0x02)
    #define GYRO_CFG2_BIT_XCT_EN                (0x20)          // 使能陀螺仪X轴自检
    #define GYRO_CFG2_BIT_YCT_EN                (0x10)          // 使能陀螺仪Y轴自检
    #define GYRO_CFG2_BIT_ZCT_EN                (0x08)          // 使能陀螺仪Z轴自检
    #define GYRO_CFG2_MSK_AVGCFG                (0x07)          // 平均滤波配置
    #define GYRO_CFG2_AVGCFG_1x                 (0x00)
    #define GYRO_CFG2_AVGCFG_2x                 (0x01)
    #define GYRO_CFG2_AVGCFG_4x                 (0x02)
    #define GYRO_CFG2_AVGCFG_8x                 (0x03)
    #define GYRO_CFG2_AVGCFG_16x                (0x04)
    #define GYRO_CFG2_AVGCFG_32x                (0x05)
    #define GYRO_CFG2_AVGCFG_64x                (0x06)
    #define GYRO_CFG2_AVGCFG_128x               (0x07)
// 陀螺仪校准数值，单位 0.0305 dps
#define ICM20948B2_0x03_XG_OFFS_USRH            (0x03)          // 陀螺仪X轴校准值高字节
#define ICM20948B2_0x04_XG_OFFS_USRL            (0x04)          // 陀螺仪X轴校准值低字节
#define ICM20948B2_0x05_YG_OFFS_USRH            (0x05)          // 陀螺仪Y轴校准值高字节
#define ICM20948B2_0x06_YG_OFFS_USRL            (0x06)          // 陀螺仪Y轴校准值低字节
#define ICM20948B2_0x07_ZG_OFFS_USRH            (0x07)          // 陀螺仪Z轴校准值高字节
#define ICM20948B2_0x08_ZG_OFFS_USRL            (0x08)          // 陀螺仪Z轴校准值低字节
// 输出数据速率对齐
#define ICM20948B2_0x09_ODR_ALIGN_EN            (0x09)
    #define ODR_ALIGN_EN_BIT                    (0x01)          
// 加速度采样率：采样率=1125/(1+div)
#define ICM20948B2_0x10_ACCEL_SAMLRT_DIV_1      (0x10)          // 加速度采样率高字节(bit0~bit3)
#define ICM20948B2_0x11_ACCEL_SMPLRT_DIV_2      (0x11)          // 加速度采样率低字节(bit0~bit7)
// 加速度中断控制
#define ICM20948B2_0x12_ACCEL_INT_CTRL          (0x12)          
    #define ACCEL_INT_CTRL_BIT_EN               (0x02)          // 使能加速度唤醒功能
    #define ACCEL_INT_CTRL_BIT_MODE_INT         (0x01)          // 设置唤醒方式；0和初始化时的值进行对比，1和上一个值进行对比
// 设置加速度唤醒阈值，单位4mg, 取值范围0~255
#define ICM20948B2_0x13_ACCEL_WOM_THR           (0x13)
// 加速度配置寄存器
#define ICM20948B2_0x14_ACCEL_CONFIG1           (0x14)
    #define ACCEL_CFG1_MSK_DLPFCFG              (0x38)          // 低通滤波参数配置(igital Low Pass Filter parameters)
    #define ACCEL_DLPFCFG_0                     (0x00)
    #define ACCEL_DLPFCFG_1                     (0x08)
    #define ACCEL_DLPFCFG_2                     (0x10)
    #define ACCEL_DLPFCFG_3                     (0x18)
    #define ACCEL_DLPFCFG_4                     (0x20)
    #define ACCEL_DLPFCFG_5                     (0x28)
    #define ACCEL_DLPFCFG_6                     (0x30)
    #define ACCEL_DLPFCFG_7                     (0x38)
    #define ACCEL_CFG1_MSK_FS_SEL               (0x06)          // 配置加速度量程
    #define ACCEL_CFG1_FS_2G                    (0x00)          // +-2g
    #define ACCEL_CFG1_FS_4G                    (0x02)          // +-4g
    #define ACCEL_CFG1_FS_8G                    (0x04)          // +-8g
    #define ACCEL_CFG1_FS_16G                   (0x06)          // +-16g
    #define ACCEL_CFG1_BIT_FCHOICE              (0x01)
#define ICM20948B2_0x15_ACCEL_CONFIG_2          (0x15)
    #define ACCEL_CFG2_BIT_AX_ST_EN             (0x10)          // 使能加速度 X 轴自检
    #define ACCEL_CFG2_BIT_AY_ST_EN             (0x08)          // 使能加速度 Y 轴自检
    #define ACCEL_CFG2_BIT_AZ_ST_EN             (0x04)          // 使能加速度 Z 轴自检
#define ICM20948B2_0x52_FSYNC_CONFIG            (0x52)
// 温度配置寄存器
#define ICM20948B2_0x53_TEMP_CONFIG             (0x53)
    #define TEMP_CONFIG_MSK_DLPFCFG             (0x07)
#define ICM20948B2_0x54_MOD_CTRL_USR            (0x54)
    #define MOD_CTRL_USER_BIT_LP_DMP_EN         (0x01)          // 在低电加速度模式下使能DMP




/////////////////////////////USER BANK 3 REGISTER/////////////////////////////////////////
// 配置ODR(数据输出频率), 当禁止陀螺仪和加速度时，采用该配置作为数据输出配置
#define ICM20948B3_0x00_I2C_MST_ODR_CONFIG      (0x00)      // 1.1kHz / (2^(bit0~bit3))
    #define I2C_MST_ODR_CONFIG                  (0x0f)
// i2c 多主模式设置寄存器
#define ICM20948B3_0x01_I2C_MST_CTRL            (0x01)
    #define I2C_MST_CTRL_BIT_EN                 (0x80)      // 使能 i2c 多主模式
    #define I2C_MST_CTRL_BIT_2C_MST_P_NSR       (0x10)      // 该位控制I2C主机从一个从读到下一个从读的转换
    #define I2C_MST_CTRL_MSK_CLK                (0x0f)      // i2c 时钟设置
#define ICM20948B3_0x02_I2C_MST_DELAY_CTRL      (0x02)
// I2C slave0 配置寄存器
#define ICM20948B3_0x03_I2C_SLV0_ADDR           (0x03)      // 选择i2c slv0
#define ICM20948B3_0x04_I2C_SLV0_REG            (0x04)      // 设置i2c slv0地址 
#define ICM20948B3_0x05_I2C_SLV0_CTRL           (0x05)      // 控制i2c slv0;
    #define I2C_SLV0_BIT_EN                     (0x80)
#define ICM20948B3_0x06_I2C_SLV0_DO             (0x06)      // 设置 i2c slv0 数据
// I2C slave1 配置寄存器
#define ICM20948B3_0x07_I2C_SLV1_ADDR           (0x07)      // 选择i2c slv1
#define ICM20948B3_0x08_I2C_SLV1_REG            (0x08)      // 设置i2c slv1地址 
#define ICM20948B3_0x09_I2C_SLV1_CTRL           (0x09)      // 控制i2c slv1;
#define ICM20948B3_0x0a_I2C_SLV1_DO             (0x0a)      // 设置 i2c slv1 数据
// I2C slave2 配置寄存器
#define ICM20948B3_0x0b_I2C_SLV2_ADDR           (0x0b)      // 选择i2c slv2
#define ICM20948B3_0x0c_I2C_SLV2_REG            (0x0c)      // 设置i2c slv2地址 
#define ICM20948B3_0x0d_I2C_SLV2_CTRL           (0x0d)      // 控制i2c slv2;
#define ICM20948B3_0x0e_I2C_SLV2_DO             (0x0e)      // 设置 i2c slv2 数据
// I2C slave3 配置寄存器
#define ICM20948B3_0x0f_I2C_SLV3_ADDR           (0x0f)      // 选择i2c slv3
#define ICM20948B3_0x10_I2C_SLV3_REG            (0x10)      // 设置i2c slv3地址 
#define ICM20948B3_0x11_I2C_SLV3_CTRL           (0x11)      // 控制i2c slv3;
#define ICM20948B3_0x12_I2C_SLV3_DO             (0x12)      // 设置 i2c slv3 数据
// I2C slave4 配置寄存器
#define ICM20948B3_0x13_I2C_SLV4_ADDR           (0x13)      // 选择i2c slv4
#define ICM20948B3_0x14_I2C_SLV4_REG            (0x14)      // 设置i2c slv4地址 
#define ICM20948B3_0x15_I2C_SLV4_CTRL           (0x15)      // 控制i2c slv4
#define ICM20948B3_0x16_I2C_SLV4_DO             (0x16)      // 设置 i2c slv4 数据
#define ICM20948B3_0x17_I2C_SLV4_DI             (0x17)      // 设置 i2c slv4 数据




/////////////////////////////AK09916 寄存器/////////////////////////////////////////
#define AK09916_0x01_WIA2                       (0x01)      // 默认 0x09
#define AK09916_0x10_ST1                        (0x10)
    #define ST1_BIT_DRDY                        (0x01)
    #define ST1_BIT_DOR                         (0x02)
#define AK09916_0x11_HXL                        (0x11)      // 地磁数据X轴低字节
#define AK09916_0x12_HXH                        (0x12)      // 地磁数据X轴高字节
#define AK09916_0x13_HYL                        (0x13)      // 地磁数据Y轴低字节
#define AK09916_0x14_HYH                        (0x14)      // 地磁数据Y轴高字节
#define AK09916_0x15_HZL                        (0x15)      // 地磁数据Z轴低字节
#define AK09916_0x16_HZH                        (0x16)      // 地磁数据Z轴高字节
#define AK09916_0x18_ST2                        (0x18)
    #define ST2_BIT_HOFL                        (0x08)      // 地磁数据溢出标记位
#define AK09916_0x31_CNTL2                      (0x31)
    #define CNTL2_MODE_POWER_DOWN               (0x00)      // 关机模式
    #define CNTL2_MODE_SINGLE_MEASURE           (0x01)      // 单次测量模式
    #define CNTL2_MODE_CONTINUE1_MEASURE        (0x02)      // 连续测量模式1, 10HZ
    #define CNTL2_MODE_CONTINUE2_MEASURE        (0x04)      // 连续测量模式2, 20HZ
    #define CNTL2_MODE_CONTINUE3_MEASURE        (0x06)      // 连续测量模式3, 50HZ
    #define CNTL2_MODE_CONTINUE4_MEASURE        (0x08)      // 连续测量模式4, 100HZ
    #define CNTL2_MODE_SELF_TEST                (0x10)      // 自测模式
#define AK09916_0x32_CNTL3                      (0x32)
    #define CNTL3_BIT_SOFT_RST                  (0x01)      // 软件复位
#define AK09916_0x33_TS1                        (0x33)
#define AK09916_0x34_TS2                        (0x34)




//////////////////////////////////////////////////////////////////////
#define GYRO_RANGE_250DPS                       GYRO_CFG1_FS_250
#define GYRO_RANGE_500DPS                       GYRO_CFG1_FS_500
#define GYRO_RANGE_1000DPS                      GYRO_CFG1_FS_1000
#define GYRO_RANGE_2000DPS                      GYRO_CFG1_FS_2000
typedef uint8_t gyro_range_t;

#define ACCEL_RANGE_2G                          ACCEL_CFG1_FS_2G
#define ACCEL_RANGE_4G                          ACCEL_CFG1_FS_4G
#define ACCEL_RANGE_8G                          ACCEL_CFG1_FS_8G
#define ACCEL_RANGE_16G                         ACCEL_CFG1_FS_16G
typedef uint8_t accel_range_t;


// 加速度值
typedef struct {
    int16_t z;
    int16_t y;
    int16_t x;
} accelerometer_t;

// 陀螺仪值
typedef struct {
    int16_t z;
    int16_t y;
    int16_t x;
} gyroscope_t;

// 地磁偏转角值
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} mag_val_t;

// 温度
typedef int16_t temperature_t;


// @brief 读取 icm20948 设备id
uint8_t drv_icm20948_who_am_i(void);
// @brief 读取 ak09916 设备 id
uint8_t drv_ak09916_who_am_i(void);
// @brief 设置加速度量程范围
void drv_icm20948_accel_range_set(accel_range_t range);
// @brief 设置陀螺仪量程范围
void drv_icm20948_gyro_range_set(gyro_range_t range);
// @brief 读取加速度原始数值
// @return 成功返回读取到的数据长度6，失败返回0
uint8_t drv_icm20948_acce_data_get(int16_t *ax, int16_t *ay, int16_t *az);
// @brief 读取陀螺仪原始数值
// @return 成功返回读取到的数据长度6，失败返回0
uint8_t drv_icm20948_gyroscope_data_get(int16_t *gx, int16_t *gy, int16_t *gz);
// @brief 读取温度
// @return 成功返回读取到的数据长度2，失败返回0
uint8_t drv_icm20948_temperature_get(int16_t* temperature);
// @brief 读取地磁数据
// @return 成功返回读取到的数据长度6，失败返回0
uint8_t drv_icm20948_mag_data_get(int16_t *mx, int16_t *my, int16_t *mz);
// @brief 读取九轴数据
int32_t drv_icm20948_9axis_data_read(accelerometer_t* p_acc, gyroscope_t* p_gyro, mag_val_t* p_mag);
// @brief 初始化 icm20948
int32_t drv_icm20948_init(void);




#ifdef __cplusplus
}
#endif
#endif /*__DRV_ICM20948_H__*/

